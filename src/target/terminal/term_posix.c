#define _DEFAULT_SOURCE

#include "../../color.h"
#include "term.h"

#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static int tty_fd = -1;

static struct termios orig_termios;
static struct termios raw_termios;
static bool have_orig_termios = false;
static volatile sig_atomic_t is_active = 0;
static volatile sig_atomic_t need_redraw = 0;
static char stdout_buf[1 << 16];

static const char enter_seq[] = "\x1b[?1049h\x1b[?25l\x1b[2J\x1b[H";
static const char leave_seq[] = "\x1b[?25h\x1b[?1049l";

static void write_tty(const char *s, size_t n) {
  while (n > 0) {
    ssize_t w = write(tty_fd, s, n);
    if (w <= 0) return;
    s += w;
    n -= (size_t)w;
  }
}

static void on_fatal_signal(int sig) {
  if (is_active) {
    write_tty(leave_seq, sizeof leave_seq - 1);
    tcsetattr(tty_fd, TCSADRAIN, &orig_termios);
  }
  raise(sig);
}

static void install(int sig, void (*fn)(int), int flags) {
  struct sigaction sa;
  memset(&sa, 0, sizeof sa);
  sa.sa_handler = fn;
  sa.sa_flags = flags;
  sigaction(sig, &sa, NULL);
}

static void on_cont_signal(int sig) {
  (void)sig;
  if (!is_active) return;
  install(SIGTSTP, on_fatal_signal, SA_RESETHAND);
  tcsetattr(tty_fd, TCSAFLUSH, &raw_termios);
  write_tty(enter_seq, sizeof enter_seq - 1);
  need_redraw = 1;
}

static void install_handlers(void) {
  install(SIGQUIT, on_fatal_signal, SA_RESETHAND);
  install(SIGHUP, on_fatal_signal, SA_RESETHAND);
  install(SIGTSTP, on_fatal_signal, SA_RESETHAND);
  install(SIGCONT, on_cont_signal, SA_RESTART);
}

static void remove_handlers(void) {
  signal(SIGQUIT, SIG_DFL);
  signal(SIGHUP, SIG_DFL);
  signal(SIGTSTP, SIG_DFL);
  signal(SIGCONT, SIG_DFL);
}

int term_init(void) {
  tty_fd = open("/dev/tty", O_RDWR);
  if (tty_fd < 0) return -1;
  if (tcgetattr(tty_fd, &orig_termios) != 0) {
    close(tty_fd);
    tty_fd = -1;
    return -1;
  }
  have_orig_termios = true;
  struct termios raw = orig_termios;
  cfmakeraw(&raw);
  raw.c_lflag |= ISIG; /* keep ctrl+c/ctrl+\ generating SIGINT/SIGQUIT */
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  raw_termios = raw;
  if (tcsetattr(tty_fd, TCSAFLUSH, &raw) != 0) {
    close(tty_fd);
    tty_fd = -1;
    have_orig_termios = false;
    return -1;
  }
  setvbuf(stdout, stdout_buf, _IOFBF, sizeof(stdout_buf));
  fputs(enter_seq, stdout);
  fflush(stdout);
  is_active = 1;
  install_handlers();
  return 0;
}

void term_shutdown(void) {
  if (!is_active) return;
  is_active = 0;
  remove_handlers();
  fputs(leave_seq, stdout);
  fflush(stdout);
  if (have_orig_termios) tcsetattr(tty_fd, TCSADRAIN, &orig_termios); /* keep typeahead */
  if (tty_fd >= 0) {
    close(tty_fd);
    tty_fd = -1;
  }
  term_common_shutdown();
}

void term_size(int *cols, int *rows) {
  struct winsize ws;
  if (ioctl(tty_fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return;
  }
  *cols = 80;
  *rows = 24;
}

bool term_has_color(void) { return color_supported(); }

static bool have_pending = false;
static unsigned char pending_byte;

static bool read_byte(int timeout_ms, unsigned char *out) {
  if (have_pending) {
    *out = pending_byte;
    have_pending = false;
    return true;
  }
  struct pollfd pfd = {.fd = tty_fd, .events = POLLIN};
  if (poll(&pfd, 1, timeout_ms) <= 0) return false;
  return read(tty_fd, out, 1) == 1;
}

int term_poll_key(int timeout_ms) {
  if (need_redraw) {
    need_redraw = 0;
    term_invalidate();
  }
  unsigned char c;
  if (!read_byte(timeout_ms, &c)) return -1;
  if (c != 0x1b) return tolower(c);

  unsigned char c2;
  if (!read_byte(25, &c2)) return 27;
  if (c2 != '[' && c2 != 'O') {
    have_pending = true;
    pending_byte = c2;
    return 27;
  }
  unsigned char c3;
  if (!read_byte(25, &c3)) return 27;
  if (c2 == '[') {
    /* CSI: params 0x30-0x3f, intermediates 0x20-0x2f, final 0x40-0x7e */
    for (int n = 0; c3 < 0x40 && n < 32; n++) {
      if (!read_byte(25, &c3)) return -1;
    }
  }
  switch (c3) {
    case 'A': return TERM_KEY_UP;
    case 'B': return TERM_KEY_DOWN;
    case 'C': return TERM_KEY_RIGHT;
    case 'D': return TERM_KEY_LEFT;
    default:  return -1;
  }
}
