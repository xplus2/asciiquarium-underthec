#ifndef UNDERTHEC_CANVAS_H
#define UNDERTHEC_CANVAS_H

#include "color.h"

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

bool canvas_diff(struct canvas *c);
/* next diff reports every cell */
void canvas_diff_invalidate(struct canvas *c);

#endif
