#ifndef UNDERTHEC_CANVAS_H
#define UNDERTHEC_CANVAS_H

#include "color.h"

#include <stdint.h>

struct cell {
  char glyph[5];
  enum color col;
  bool bold;
  bool cont; /* 2nd multibyte */
  enum color bg;
  bool bg_bold;
  bool in_touched;
  bool in_active;
};

struct canvas {
  int width;
  int height;
  struct cell *cells;
  int *touched;
  int touched_count;
  int touched_cap;
  int *active;
  int active_count;
  int active_cap;
  struct cell *prev;
  int *row_lo;
  int *row_hi;
  unsigned *changed_at; /* diff_seq of last change */
  unsigned diff_seq;
  bool diff_all;
  bool diff_full;
  bool diff_enabled;
  bool diff_consumed;
};

/* w/h may start being 0 */
void canvas_init(struct canvas *c);
void canvas_free(struct canvas *c);
void canvas_resize(struct canvas *c, int width, int height);
void canvas_clear(struct canvas *c);
void canvas_put(struct canvas *c, int x, int y, const char *glyph, int glyph_len, struct attr a, int cols);
void canvas_reduce_colors(struct canvas *c, int colors_mode);
/* w x h blanks */
void canvas_fill(struct canvas *c, int x0, int y0, int w, int h, struct attr a);
/* ASCII s[start, start + len) at x0 + index. len < 0: to end */
void canvas_text_span(struct canvas *c, int x0, int y, const char *s, int start, int len, struct attr a);
static inline void canvas_text(struct canvas *c, int x0, int y, const char *s, struct attr a) {
  canvas_text_span(c, x0, y, s, 0, -1, a);
}

/* per cell: cp << 10 | bg_bold << 9 | bg << 5 | col << 1 | bold. cont: cp 0. out holds w*h */
void canvas_pack(const struct canvas *c, uint32_t *out);

/* same fg col+bold, cols [col0, col1), trailing cont cells included */
typedef void (*canvas_run_fn)(void *ctx, int row, int col0, int col1, const struct cell *attr);
/* cols lo..hi, runs of at most max_cells glyphs */
void canvas_row_runs(const struct canvas *c, int row, int lo, int hi, int max_cells, canvas_run_fn fn, void *ctx);
/* glyph bytes of cols [col0, col1), no NUL. ret len */
int canvas_run_utf8(const struct canvas *c, int row, int col0, int col1, char *out, int out_cap);

bool canvas_diff(struct canvas *c);
/* after canvas_diff: cell differs from previous frame */
static inline bool canvas_cell_changed(const struct canvas *c, int idx) {
  return c->diff_all || c->changed_at[idx] == c->diff_seq;
}
/* next diff reports every cell */
void canvas_diff_invalidate(struct canvas *c);

#endif
