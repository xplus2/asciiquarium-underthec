#include "../../color.h"
#include "term.h"

#include <stdio.h>

static bool force_full = true;
static int last_w = -1;
static int last_h = -1;
static bool transparent = false;

void term_common_shutdown(void) {
  force_full = true;
  last_w = -1;
  last_h = -1;
}

void term_invalidate(void) { force_full = true; }

void term_set_transparent(bool on) {
  transparent = on;
  force_full = true; /* repaint bg */
}

static const char *const sgr_table[2][2][9] = {
  {
    {"\x1b[0;39;40m", "\x1b[0;30;40m", "\x1b[0;31;40m", "\x1b[0;32;40m", "\x1b[0;33;40m",
     "\x1b[0;34;40m", "\x1b[0;35;40m", "\x1b[0;36;40m", "\x1b[0;37;40m"},
    {"\x1b[0;1;39;40m", "\x1b[0;1;30;40m", "\x1b[0;1;31;40m", "\x1b[0;1;32;40m", "\x1b[0;1;33;40m",
     "\x1b[0;1;34;40m", "\x1b[0;1;35;40m", "\x1b[0;1;36;40m", "\x1b[0;1;37;40m"},
  },
  {
    {"\x1b[0;39m", "\x1b[0;30m", "\x1b[0;31m", "\x1b[0;32m", "\x1b[0;33m",
     "\x1b[0;34m", "\x1b[0;35m", "\x1b[0;36m", "\x1b[0;37m"},
    {"\x1b[0;1;39m", "\x1b[0;1;30m", "\x1b[0;1;31m", "\x1b[0;1;32m", "\x1b[0;1;33m",
     "\x1b[0;1;34m", "\x1b[0;1;35m", "\x1b[0;1;36m", "\x1b[0;1;37m"},
  },
};

static char *append_uint(char *p, unsigned v) {
  char tmp[10];
  int n = 0;
  do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v);
  while (n > 0) *p++ = tmp[--n];
  return p;
}

static void write_sgr(enum color col, bool bold, enum color bg, bool bg_bold, bool mono) {
  if (mono) return;
  if (bg == COL_DEFAULT) {
    fputs(sgr_table[transparent][bold][col], stdout);
    return;
  }
  char buf[24];
  char *p = buf;
  *p++ = '\x1b';
  *p++ = '[';
  *p++ = '0';
  if (bold) { *p++ = ';'; *p++ = '1'; }
  *p++ = ';';
  if (col == COL_DEFAULT) { *p++ = '3'; *p++ = '9'; }
  else p = append_uint(p, 30u + (col - 1));
  *p++ = ';';
  p = append_uint(p, (bg_bold ? 100u : 40u) + (bg - 1));
  *p++ = 'm';
  fwrite(buf, 1, (size_t)(p - buf), stdout);
}

static void write_cursor_pos(int row, int col) {
  char buf[24];
  char *p = buf;
  *p++ = '\x1b';
  *p++ = '[';
  p = append_uint(p, (unsigned)row);
  *p++ = ';';
  p = append_uint(p, (unsigned)col);
  *p++ = 'H';
  fwrite(buf, 1, (size_t)(p - buf), stdout);
}

void term_present(struct canvas *c) {
  bool mono = !term_has_color();
  if (force_full || c->width != last_w || c->height != last_h) {
    canvas_diff_invalidate(c);
    fputs("\x1b[2J", stdout);
    force_full = false;
    last_w = c->width;
    last_h = c->height;
  }
  if (!canvas_diff(c)) {
    fflush(stdout);
    return;
  }
  for (int y = 0; y < c->height; y++) {
    int base = y * c->width;
    const struct cell *line = &c->cells[base];
    int x = c->row_lo[y];
    while (x <= c->row_hi[y]) {
      if (!canvas_cell_changed(c, base + x)) {
        x++;
        continue;
      }
      /* run of changed cells, starts on wide lead */
      int start = line[x].cont && x > 0 ? x - 1 : x;
      write_cursor_pos(y + 1, start + 1);
      const struct cell *last = NULL;
      for (x = start; x <= c->row_hi[y]; x++) {
        const struct cell *cur = &line[x];
        if (cur->cont) continue;
        if (x != start && !canvas_cell_changed(c, base + x)) break;
        if (last == NULL || cur->col != last->col || cur->bold != last->bold || cur->bg != last->bg || cur->bg_bold != last->bg_bold) {
          write_sgr(cur->col, cur->bold, cur->bg, cur->bg_bold, mono);
        }
        last = cur;
        fputs(cur->glyph, stdout);
      }
    }
  }
  fflush(stdout);
}
