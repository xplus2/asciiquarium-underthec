#include "priv.h"
#include "xalloc.h"

#include <string.h>

static bool bbox_overlap(struct entity *a, struct entity *b) {
  int aw = entity_width(a);
  int ah = entity_height(a);
  int bw = entity_width(b);
  int bh = entity_height(b);
  return a->x < b->x + bw && b->x < a->x + aw && a->y < b->y + bh && b->y < a->y + ah;
}

static int *g_teeth_idx = NULL;
static int *g_wl_idx = NULL;
static int g_idx_cap = 0;

void entity_collide_shutdown(void) {
  free(g_teeth_idx);
  free(g_wl_idx);
  g_teeth_idx = NULL;
  g_wl_idx = NULL;
  g_idx_cap = 0;
}

void entity_collide_all(struct entity_list *list, uint64_t present) {
  int n = list->count;
  bool fish_vs_teeth = (present & ENT_BIT(ENT_FISH)) && (present & ENT_BIT(ENT_TEETH));
  bool bubble_vs_wl = (present & ENT_BIT(ENT_BUBBLE)) && (present & ENT_BIT(ENT_WATERLINE));
  if (n <= 0 || !(fish_vs_teeth || bubble_vs_wl)) return;
  if (n > g_idx_cap) {
    g_teeth_idx = xrealloc(g_teeth_idx, (size_t)n * sizeof(*g_teeth_idx));
    g_wl_idx = xrealloc(g_wl_idx, (size_t)n * sizeof(*g_wl_idx));
    g_idx_cap = n;
  }
  int teeth_n = 0;
  int wl_n = 0;
  for (int j = 0; j < n; j++) {
    const struct entity *e = &list->items[j];
    if (e->marked_dead || !e->physical) continue;
    if (e->type == ENT_TEETH) g_teeth_idx[teeth_n++] = j;
    else if (e->type == ENT_WATERLINE) g_wl_idx[wl_n++] = j;
  }

  for (int i = 0; fish_vs_teeth && i < n; i++) {
    struct entity *fish = &list->items[i];
    if (fish->marked_dead || !fish->physical || fish->type != ENT_FISH) continue;
    if (entity_height(fish) > 5) continue;
    for (int k = 0; k < teeth_n; k++) {
      struct entity *teeth = &list->items[g_teeth_idx[k]];
      if (bbox_overlap(fish, teeth)) {
        fish->marked_dead = true;
        fish->spawn_splat = true;
        fish->splat_x = teeth->x;
        fish->splat_y = teeth->y;
        fish->splat_z = teeth->z;
        break;
      }
    }
  }
  for (int i = 0; bubble_vs_wl && i < n; i++) {
    struct entity *bubble = &list->items[i];
    if (bubble->marked_dead || !bubble->physical || bubble->type != ENT_BUBBLE) continue;
    for (int k = 0; k < wl_n; k++) {
      struct entity *wl = &list->items[g_wl_idx[k]];
      if (bbox_overlap(bubble, wl)) {
        bubble->marked_dead = true;
        break;
      }
    }
  }
}

void row_view_init(struct row_view *v, const struct entity *e, const char *srow) {
  v->s = srow;
  v->bytes = (int)strlen(srow);
  v->cols = utf8_col_width(srow);
  v->start = 0;
  v->end = v->bytes;
  if (e->trim_edges) row_trim(srow, v->bytes, &v->start, &v->end);
}

int row_view_offset(const struct row_view *v, int col) {
  return v->bytes == v->cols ? col : utf8_byte_offset(v->s, col);
}

bool row_view_transparent_at(const struct row_view *v, const struct entity *e, int off) {
  char ch = v->s[off];
  if (e->sentinel != 0 && ch == e->sentinel) return true;
  return e->trim_edges && ch == ' ' && (off < v->start || off >= v->end);
}

/* placed rows, style: trim_edges + sentinel source */
struct glyph_box {
  ascii_rows rows;
  int x;
  int y;
  int w;
  int h;
  const struct entity *style;
};

static bool boxes_overlap(const struct glyph_box *a, const struct glyph_box *b) {
  int x0 = a->x > b->x ? a->x : b->x;
  int x1 = (a->x + a->w) < (b->x + b->w) ? (a->x + a->w) : (b->x + b->w);
  int y0 = a->y > b->y ? a->y : b->y;
  int y1 = (a->y + a->h) < (b->y + b->h) ? (a->y + a->h) : (b->y + b->h);
  for (int wy = y0; wy < y1; wy++) {
    int arow = wy - a->y;
    int brow = wy - b->y;
    if (arow < 0 || arow >= a->h || brow < 0 || brow >= b->h) continue;
    if (a->rows[arow] == NULL || b->rows[brow] == NULL) continue;
    struct row_view av;
    struct row_view bv;
    row_view_init(&av, a->style, a->rows[arow]);
    row_view_init(&bv, b->style, b->rows[brow]);
    for (int wx = x0; wx < x1; wx++) {
      int acol = wx - a->x;
      int bcol = wx - b->x;
      if (acol < 0 || acol >= av.cols || bcol < 0 || bcol >= bv.cols) continue;
      if (row_view_transparent_at(&av, a->style, row_view_offset(&av, acol))) continue;
      if (row_view_transparent_at(&bv, b->style, row_view_offset(&bv, bcol))) continue;
      return true;
    }
  }
  return false;
}

/* false: no shape */
static bool entity_box(struct glyph_box *g, struct entity *e) {
  g->rows = entity_shape(e);
  if (g->rows == NULL) return false;
  g->x = round_to_int(e->x);
  g->y = round_to_int(e->y);
  g->w = entity_width(e);
  g->h = entity_height(e);
  g->style = e;
  return true;
}

bool entity_glyph_overlap(struct entity *a, struct entity *b) {
  struct glyph_box ga;
  struct glyph_box gb;
  if (!entity_box(&ga, a) || !entity_box(&gb, b)) return false;
  return boxes_overlap(&ga, &gb);
}

bool entity_rows_overlap(ascii_rows rows, double x, double y, const struct entity *style, struct entity *b) {
  struct glyph_box ga = {rows, round_to_int(x), round_to_int(y), 0, 0, style};
  for (; rows[ga.h] != NULL; ga.h++) {
    int len = utf8_col_width(rows[ga.h]);
    if (len > ga.w) ga.w = len;
  }
  struct glyph_box gb;
  if (!entity_box(&gb, b)) return false;
  return boxes_overlap(&ga, &gb);
}
