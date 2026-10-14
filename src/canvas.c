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
  c->prev = NULL;
  c->row_lo = NULL;
  c->row_hi = NULL;
  c->diff_full = true;
  c->diff_enabled = false;
  c->diff_consumed = true;
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
  free(c->prev);
  free(c->row_lo);
  free(c->row_hi);
  c->prev = NULL;
  c->row_lo = NULL;
  c->row_hi = NULL;
  c->diff_full = true;
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
  free(c->prev);
  free(c->row_lo);
  free(c->row_hi);
  c->prev = NULL;
  c->row_lo = NULL;
  c->row_hi = NULL;
  c->diff_full = true;
  size_t n = (size_t)width * (size_t)height;
  c->cells = n > 0 ? xcalloc(n, sizeof(*c->cells)) : NULL;
  c->width = width;
  c->height = height;
  c->touched_count = 0;
  c->active_count = 0;
  canvas_fill_blank(c);
}

void canvas_clear(struct canvas *c) {
  if (c->diff_enabled && !c->diff_consumed) c->diff_full = true;
  c->diff_consumed = false;
  struct cell blank = {.glyph = {' ', '\0'}, .col = COL_DEFAULT, .bold = false, .cont = false};
  for (int i = 0; i < c->touched_count; i++) c->cells[c->touched[i]].in_touched = false;
  for (int i = 0; i < c->active_count; i++) {
    struct cell *cell = &c->cells[c->active[i]];
    *cell = blank;
    cell->in_touched = true;
  }
  if (c->active_count > c->touched_cap) {
    c->touched = xrealloc(c->touched, (size_t)c->active_count * sizeof(*c->touched));
    c->touched_cap = c->active_count;
  }
  if (c->active_count > 0) memcpy(c->touched, c->active, (size_t)c->active_count * sizeof(*c->touched));
  c->touched_count = c->active_count;
  c->active_count = 0;
}

static void canvas_mark(struct canvas *c, int idx) {
  struct cell *cell = &c->cells[idx];
  if (!cell->in_touched) {
    if (c->touched_count == c->touched_cap) {
      int newcap = c->touched_cap ? c->touched_cap * 2 : 64;
      c->touched = xrealloc(c->touched, (size_t)newcap * sizeof(*c->touched));
      c->touched_cap = newcap;
    }
    c->touched[c->touched_count++] = idx;
    cell->in_touched = true;
  }
  if (!cell->in_active) {
    if (c->active_count == c->active_cap) {
      int newcap = c->active_cap ? c->active_cap * 2 : 64;
      c->active = xrealloc(c->active, (size_t)newcap * sizeof(*c->active));
      c->active_cap = newcap;
    }
    c->active[c->active_count++] = idx;
    cell->in_active = true;
  }
}

static void canvas_blank_cell(struct canvas *c, int idx, struct attr a) {
  struct cell *cell = &c->cells[idx];
  cell->glyph[0] = ' ';
  cell->glyph[1] = '\0';
  cell->col = a.col;
  cell->bold = a.bold;
  cell->bg = a.bg;
  cell->bg_bold = a.bg_bold;
  cell->cont = false;
  canvas_mark(c, idx);
}

static void canvas_split_wide(struct canvas *c, int x, int idx, struct attr a) {
  if (c->cells[idx].cont) {
    if (x > 0) canvas_blank_cell(c, idx - 1, a);
  } else if (x + 1 < c->width && c->cells[idx + 1].cont) {
    canvas_blank_cell(c, idx + 1, a);
  }
}

void canvas_put(struct canvas *c, int x, int y, const char *glyph, int glyph_len, struct attr a, int cols) {
  if (x < 0 || y < 0 || x >= c->width || y >= c->height) return;
  if (glyph_len < 1) glyph_len = 1;
  if (glyph_len > 4) glyph_len = 4;
  if (cols < 1) cols = 1;
  int idx = y * c->width + x;
  if (cols >= 2 && x + 1 >= c->width) {
    canvas_split_wide(c, x, idx, a);
    canvas_blank_cell(c, idx, a);
    return;
  }
  canvas_split_wide(c, x, idx, a);
  if (cols >= 2) canvas_split_wide(c, x + 1, idx + 1, a);
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
  for (int i = 0; i < c->active_count; i++) {
    struct cell *cell = &c->cells[c->active[i]];
    color_reduce(colors_mode, &cell->col, &cell->bold);
  }
}

static bool cells_differ(const struct cell *a, const struct cell *b) {
  return a->cont != b->cont || a->col != b->col || a->bold != b->bold || a->bg != b->bg ||
         a->bg_bold != b->bg_bold || strcmp(a->glyph, b->glyph) != 0;
}

void canvas_diff_invalidate(struct canvas *c) {
  c->diff_full = true;
}

bool canvas_diff(struct canvas *c) {
  if (c->width <= 0 || c->height <= 0 || c->cells == NULL) return false;
  size_t n = (size_t)c->width * (size_t)c->height;
  if (c->prev == NULL) {
    c->prev = xcalloc(n, sizeof(*c->prev));
    c->row_lo = xcalloc((size_t)c->height, sizeof(*c->row_lo));
    c->row_hi = xcalloc((size_t)c->height, sizeof(*c->row_hi));
    c->diff_full = true;
  }
  c->diff_enabled = true;
  c->diff_consumed = true;
  if (c->diff_full) {
    memcpy(c->prev, c->cells, n * sizeof(*c->prev));
    for (int y = 0; y < c->height; y++) {
      c->row_lo[y] = 0;
      c->row_hi[y] = c->width - 1;
    }
    c->diff_full = false;
    return true;
  }
  for (int y = 0; y < c->height; y++) {
    c->row_lo[y] = c->width;
    c->row_hi[y] = -1;
  }
  bool any = false;
  for (int i = 0; i < c->touched_count; i++) {
    int idx = c->touched[i];
    if (!cells_differ(&c->cells[idx], &c->prev[idx])) continue;
    c->prev[idx] = c->cells[idx];
    int y = idx / c->width;
    int x = idx % c->width;
    if (x < c->row_lo[y]) c->row_lo[y] = x;
    if (x > c->row_hi[y]) c->row_hi[y] = x;
    any = true;
  }
  if (!any) return false;
  for (int y = 0; y < c->height; y++) {
    if (c->row_hi[y] < c->row_lo[y]) continue;
    const struct cell *line = &c->cells[(size_t)y * (size_t)c->width];
    int lo = c->row_lo[y] > 0 ? c->row_lo[y] - 1 : 0;
    int hi = c->row_hi[y] + 1 < c->width ? c->row_hi[y] + 1 : c->width - 1;
    while (lo > 0 && line[lo].cont) lo--;
    if (hi + 1 < c->width && line[hi + 1].cont) hi++;
    c->row_lo[y] = lo;
    c->row_hi[y] = hi;
  }
  return true;
}
