#include "../../color.h"
#include "../../xalloc.h"
#include "term.h"

#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/farptr.h>
#include <time.h>

#define BDA_MODE    0x449
#define BDA_COLS    0x44a
#define BDA_CURSOR  0x460
#define BDA_ROWS    0x484
#define TEXT_BASE   0xb8000u
#define KEY_VIDEO   'v'
#define KEY_CTRL_C  3

enum scan_lines { SCAN_KEEP, SCAN_350, SCAN_400 };
enum font_rom { FONT_KEEP, FONT_8X8 };
enum adapter { ADAPTER_CGA, ADAPTER_EGA, ADAPTER_VGA };

struct text_mode {
  int cols;
  int rows;
  int bios_mode;
  enum adapter need;
  enum scan_lines vga_scan;
  enum font_rom font;
};

static const struct text_mode modes[] = {
  {80, 25, 3, ADAPTER_CGA, SCAN_400, FONT_KEEP},
  {80, 43, 3, ADAPTER_EGA, SCAN_350, FONT_8X8},
  {80, 50, 3, ADAPTER_VGA, SCAN_400, FONT_8X8},
  {40, 25, 1, ADAPTER_CGA, SCAN_400, FONT_KEEP},
};
#define MODE_COUNT ((int)(sizeof modes / sizeof modes[0]))

static const uint8_t vga_color[9] = {7, 0, 4, 2, 6, 1, 5, 3, 7};

struct cp437_map {
  uint16_t cp;
  uint8_t byte;
};

static const struct cp437_map cp437[] = {
  {0x00a0, 0xff}, {0x00a1, 0xad}, {0x00a2, 0x9b}, {0x00a3, 0x9c}, {0x00a5, 0x9d}, {0x00aa, 0xa6},
  {0x00ab, 0xae}, {0x00ac, 0xaa}, {0x00b0, 0xf8}, {0x00b1, 0xf1}, {0x00b2, 0xfd}, {0x00b5, 0xe6},
  {0x00b7, 0xfa}, {0x00ba, 0xa7}, {0x00bb, 0xaf}, {0x00bc, 0xac}, {0x00bd, 0xab}, {0x00bf, 0xa8},
  {0x00c4, 0x8e}, {0x00c5, 0x8f}, {0x00c6, 0x92}, {0x00c7, 0x80}, {0x00c9, 0x90}, {0x00d1, 0xa5},
  {0x00d6, 0x99}, {0x00dc, 0x9a}, {0x00df, 0xe1}, {0x00e0, 0x85}, {0x00e1, 0xa0}, {0x00e2, 0x83},
  {0x00e4, 0x84}, {0x00e5, 0x86}, {0x00e6, 0x91}, {0x00e7, 0x87}, {0x00e8, 0x8a}, {0x00e9, 0x82},
  {0x00ea, 0x88}, {0x00eb, 0x89}, {0x00ec, 0x8d}, {0x00ed, 0xa1}, {0x00ee, 0x8c}, {0x00ef, 0x8b},
  {0x00f1, 0xa4}, {0x00f2, 0x95}, {0x00f3, 0xa2}, {0x00f4, 0x93}, {0x00f6, 0x94}, {0x00f7, 0xf6},
  {0x00f9, 0x97}, {0x00fa, 0xa3}, {0x00fb, 0x96}, {0x00fc, 0x81}, {0x00ff, 0x98}, {0x0192, 0x9f},
  {0x0393, 0xe2}, {0x0398, 0xe9}, {0x03a3, 0xe4}, {0x03a6, 0xe8}, {0x03a9, 0xea}, {0x03b1, 0xe0},
  {0x03b4, 0xeb}, {0x03b5, 0xee}, {0x03c0, 0xe3}, {0x03c3, 0xe5}, {0x03c4, 0xe7}, {0x03c6, 0xed},
  {0x207f, 0xfc}, {0x2219, 0xf9}, {0x221a, 0xfb}, {0x221e, 0xec}, {0x2229, 0xef}, {0x2248, 0xf7},
  {0x2261, 0xf0}, {0x2264, 0xf3}, {0x2265, 0xf2}, {0x2310, 0xa9}, {0x2320, 0xf4}, {0x2321, 0xf5},
  {0x2500, 0xc4}, {0x2502, 0xb3}, {0x250c, 0xda}, {0x2510, 0xbf}, {0x2514, 0xc0}, {0x2518, 0xd9},
  {0x251c, 0xc3}, {0x2524, 0xb4}, {0x252c, 0xc2}, {0x2534, 0xc1}, {0x253c, 0xc5}, {0x2550, 0xcd},
  {0x2551, 0xba}, {0x2552, 0xd5}, {0x2553, 0xd6}, {0x2554, 0xc9}, {0x2555, 0xb8}, {0x2556, 0xb7},
  {0x2557, 0xbb}, {0x2558, 0xd4}, {0x2559, 0xd3}, {0x255a, 0xc8}, {0x255b, 0xbe}, {0x255c, 0xbd},
  {0x255d, 0xbc}, {0x255e, 0xc6}, {0x255f, 0xc7}, {0x2560, 0xcc}, {0x2561, 0xb5}, {0x2562, 0xb6},
  {0x2563, 0xb9}, {0x2564, 0xd1}, {0x2565, 0xd2}, {0x2566, 0xcb}, {0x2567, 0xcf}, {0x2568, 0xd0},
  {0x2569, 0xca}, {0x256a, 0xd8}, {0x256b, 0xd7}, {0x256c, 0xce}, {0x2580, 0xdf}, {0x2584, 0xdc},
  {0x2588, 0xdb}, {0x258c, 0xdd}, {0x2590, 0xde}, {0x2591, 0xb0}, {0x2592, 0xb1}, {0x2593, 0xb2},
  {0x25a0, 0xfe},
};
#define CP437_COUNT ((int)(sizeof cp437 / sizeof cp437[0]))

static enum adapter adapter_kind = ADAPTER_CGA;
static bool is_active = false;
static int mode_idx = 0;
static int orig_mode;
static int orig_cols;
static int orig_rows;
static unsigned orig_cursor;
static uint16_t *prev = NULL;
static size_t prev_cap = 0;
static bool prev_valid = false;

static void bios_video(int ax, int bx, int cx) {
  __dpmi_regs r = {0};
  r.x.ax = (unsigned short)ax;
  r.x.bx = (unsigned short)bx;
  r.x.cx = (unsigned short)cx;
  __dpmi_int(0x10, &r);
}

static enum adapter detect_adapter(void) {
  __dpmi_regs r = {0};
  r.x.ax = 0x1a00;
  __dpmi_int(0x10, &r);
  if (r.h.al == 0x1a) return ADAPTER_VGA;
  r.h.ah = 0x12;
  r.h.bl = 0x10;
  __dpmi_int(0x10, &r);
  return r.h.bl != 0x10 ? ADAPTER_EGA : ADAPTER_CGA;
}

static int bda_rows(void) {
  if (adapter_kind == ADAPTER_CGA) return 25;
  int r = _farpeekb(_dos_ds, BDA_ROWS) + 1;
  return r > 1 ? r : 25;
}

static bool mode_available(const struct text_mode *m) { return adapter_kind >= m->need; }

static void apply_mode(const struct text_mode *m) {
  if (adapter_kind == ADAPTER_VGA) bios_video(m->vga_scan == SCAN_350 ? 0x1201 : 0x1202, 0x30, 0);
  bios_video(m->bios_mode, 0, 0);
  if (m->font == FONT_8X8) bios_video(0x1112, 0, 0);
  if (adapter_kind != ADAPTER_CGA) bios_video(0x1003, 0, 0);
  bios_video(0x0100, 0, 0x2000);
  prev_valid = false;
}

static int find_mode(int cols, int rows) {
  for (int i = 0; i < MODE_COUNT; i++) {
    if (modes[i].cols == cols && modes[i].rows == rows && mode_available(&modes[i])) return i;
  }
  return -1;
}

static void next_mode(void) {
  for (int i = 1; i <= MODE_COUNT; i++) {
    int idx = (mode_idx + i) % MODE_COUNT;
    if (!mode_available(&modes[idx])) continue;
    mode_idx = idx;
    apply_mode(&modes[idx]);
    return;
  }
}

int term_init(void) {
  if (_farpeekb(_dos_ds, BDA_MODE) == 7) return -1;
  adapter_kind = detect_adapter();
  orig_mode = _farpeekb(_dos_ds, BDA_MODE);
  orig_cols = _farpeekw(_dos_ds, BDA_COLS);
  orig_rows = bda_rows();
  orig_cursor = _farpeekw(_dos_ds, BDA_CURSOR);
  mode_idx = find_mode(orig_cols, orig_rows);
  if (mode_idx < 0) {
    mode_idx = 0;
    apply_mode(&modes[0]);
  } else {
    apply_mode(&modes[mode_idx]);
  }
  is_active = true;
  return 0;
}

void term_shutdown(void) {
  if (!is_active) return;
  is_active = false;
  int idx = find_mode(orig_cols, orig_rows);
  if (idx >= 0) {
    apply_mode(&modes[idx]);
  } else {
    bios_video(orig_mode, 0, 0);
  }
  if (adapter_kind != ADAPTER_CGA) bios_video(0x1003, 1, 0);
  bios_video(0x0100, 0, (int)orig_cursor);
  free(prev);
  prev = NULL;
  prev_cap = 0;
  prev_valid = false;
}

void term_size(int *cols, int *rows) {
  *cols = _farpeekw(_dos_ds, BDA_COLS);
  *rows = bda_rows();
  if (*cols <= 0) *cols = 80;
}

bool term_has_color(void) { return true; }

void term_set_transparent(bool on) { (void)on; }

static uint8_t utf8_to_cp437(const char *g) {
  unsigned char c = (unsigned char)g[0];
  if (c >= 0x20 && c < 0x7f) return c;
  if (c < 0x80) return ' ';
  unsigned cp;
  if ((c & 0xe0) == 0xc0 && g[1] != '\0') cp = (unsigned)(c & 0x1f) << 6 | ((unsigned char)g[1] & 0x3f);
  else if ((c & 0xf0) == 0xe0 && g[1] != '\0' && g[2] != '\0') cp = (unsigned)(c & 0x0f) << 12 | ((unsigned)((unsigned char)g[1] & 0x3f)) << 6 | ((unsigned char)g[2] & 0x3f);
  else return '?';
  int lo = 0;
  int hi = CP437_COUNT - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    if (cp437[mid].cp == cp) return cp437[mid].byte;
    if (cp437[mid].cp < cp) lo = mid + 1;
    else hi = mid - 1;
  }
  return '?';
}

static uint8_t attr_byte(const struct cell *c) {
  int fg = c->col == COL_DEFAULT ? 7 : vga_color[c->col];
  int bg = c->bg == COL_DEFAULT ? 0 : vga_color[c->bg];
  if (c->bold) fg |= 8;
  if (c->bg_bold && adapter_kind != ADAPTER_CGA) bg |= 8;
  return (uint8_t)(bg << 4 | fg);
}

void term_present(const struct canvas *c) {
  int cols = _farpeekw(_dos_ds, BDA_COLS);
  if (c->width != cols || c->height != bda_rows()) {
    prev_valid = false; /* clear canvas after mode switch */
    return;
  }
  size_t n = (size_t)c->width * (size_t)c->height;
  if (n > prev_cap) {
    prev = xrealloc(prev, n * sizeof *prev);
    prev_cap = n;
    prev_valid = false;
  }
  if (!prev_valid) {
    for (size_t i = 0; i < n; i++) prev[i] = 0xffff;
    prev_valid = true;
  }
  _farsetsel(_dos_ds);
  for (int y = 0; y < c->height; y++) {
    for (int x = 0; x < c->width; x++) {
      size_t i = (size_t)y * (size_t)c->width + (size_t)x;
      const struct cell *cell = &c->cells[i];
      uint8_t ch = cell->cont ? ' ' : utf8_to_cp437(cell->glyph);
      uint16_t word = (uint16_t)(attr_byte(cell) << 8 | ch);
      if (word == prev[i]) continue;
      prev[i] = word;
      _farnspokew(TEXT_BASE + ((unsigned)y * (unsigned)cols + (unsigned)x) * 2u, word);
    }
  }
}

static int read_key(void) {
  int c = getch();
  if (c == 0 || c == 0xe0) {
    switch (getch()) {
      case 72: return TERM_KEY_UP;
      case 80: return TERM_KEY_DOWN;
      case 75: return TERM_KEY_LEFT;
      case 77: return TERM_KEY_RIGHT;
      default: return -1;
    }
  }
  if (c == KEY_CTRL_C) return 'q';
  return tolower(c);
}

int term_poll_key(int timeout_ms) {
  uclock_t end = uclock() + (uclock_t)timeout_ms * UCLOCKS_PER_SEC / 1000;
  while (!kbhit()) {
    if (uclock() >= end) return -1;
    __dpmi_yield();
  }
  int key = read_key();
  if (key == KEY_VIDEO) {
    next_mode();
    return -1;
  }
  return key;
}
