#include "canvas.h"
#include "xalloc.h"

#include <stdlib.h>
#include <string.h>

void canvas_init(struct canvas *c) {
  c->width = 0;
  c->height = 0;
  c->cells = NULL;
  c->touched = NULL;
  c->touched_count = 0;
  c->touched_cap = 0;
  c->active = NULL;
  c->active_count = 0;
  c->active_cap = 0;
}

void canvas_free(struct canvas *c) {
  free(c->cells);
  c->cells = NULL;
  c->width = 0;
  c->height = 0;
  free(c->touched);
  c->touched = NULL;
  c->touched_count = 0;
  c->touched_cap = 0;
  free(c->active);
  c->active = NULL;
  c->active_count = 0;
  c->active_cap = 0;
}

static void canvas_fill_blank(struct canvas *c) {
  struct cell blank = {.glyph = {' ', '\0'}, .col = COL_DEFAULT, .bold = false, .cont = false};
  int n = c->width * c->height;
  for (int i = 0; i < n; i++) c->cells[i] = blank;
}

void canvas_resize(struct canvas *c, int width, int height) {
  if (width < 0)  width = 0;
  if (height < 0) height = 0;
  if (width == c->width && height == c->height) return;

  free(c->cells);
  size_t n = (size_t)width * (size_t)height;
  c->cells = n > 0 ? xcalloc(n, sizeof(*c->cells)) : NULL;
  c->width = width;
  c->height = height;
  c->touched_count = 0;
  c->active_count = 0;
  canvas_fill_blank(c);
}

void canvas_clear(struct canvas *c) {
  struct cell blank = {.glyph = {' ', '\0'}, .col = COL_DEFAULT, .bold = false, .cont = false};
  for (int i = 0; i < c->active_count; i++) c->cells[c->active[i]] = blank;
  if (c->active_count > c->touched_cap) {
    c->touched = xrealloc(c->touched, (size_t)c->active_count * sizeof(*c->touched));
    c->touched_cap = c->active_count;
  }
  if (c->active_count > 0) memcpy(c->touched, c->active, (size_t)c->active_count * sizeof(*c->touched));
  c->touched_count = c->active_count;
  c->active_count = 0;
}

static void canvas_mark(struct canvas *c, int idx) {
  if (c->touched_count == c->touched_cap) {
    int newcap = c->touched_cap ? c->touched_cap * 2 : 64;
    c->touched = xrealloc(c->touched, (size_t)newcap * sizeof(*c->touched));
    c->touched_cap = newcap;
  }
  c->touched[c->touched_count++] = idx;
  if (c->active_count == c->active_cap) {
    int newcap = c->active_cap ? c->active_cap * 2 : 64;
    c->active = xrealloc(c->active, (size_t)newcap * sizeof(*c->active));
    c->active_cap = newcap;
  }
  c->active[c->active_count++] = idx;
}

void canvas_put(struct canvas *c, int x, int y, const char *glyph, int glyph_len, struct attr a, int cols) {
  if (x < 0 || y < 0 || x >= c->width || y >= c->height) return;
  if (glyph_len < 1) glyph_len = 1;
  if (glyph_len > 4) glyph_len = 4;
  if (cols < 1) cols = 1;
  int idx = y * c->width + x;
  struct cell *cell = &c->cells[idx];
  memcpy(cell->glyph, glyph, (size_t)glyph_len);
  cell->glyph[glyph_len] = '\0';
  cell->col = a.col;
  cell->bold = a.bold;
  cell->bg = a.bg;
  cell->bg_bold = a.bg_bold;
  cell->cont = false;
  canvas_mark(c, idx);
  if (cols >= 2 && x + 1 < c->width) {
    struct cell *next = &c->cells[idx + 1];
    next->glyph[0] = '\0';
    next->col = a.col;
    next->bold = a.bold;
    next->bg = a.bg;
    next->bg_bold = a.bg_bold;
    next->cont = true;
    canvas_mark(c, idx + 1);
  }
}

void canvas_reduce_colors(struct canvas *c, int colors_mode) {
  if (colors_mode == 16) return;
  int n = c->width * c->height;
  for (int i = 0; i < n; i++) color_reduce(colors_mode, &c->cells[i].col, &c->cells[i].bold);
}
