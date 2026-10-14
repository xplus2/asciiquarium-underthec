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

bool cell_transparent(const struct entity *e, const char *srow, int len, int col) {
  char ch = srow[col];
  if (e->sentinel != 0 && ch == e->sentinel) return true;
  if (e->trim_edges && ch == ' ') {
    int start = 0;
    while (start < len && srow[start] == ' ') start++;
    int end = len;
    while (end > start && srow[end - 1] == ' ') end--;
    if (col < start || col >= end) return true;
  }
  return false;
}

struct row_view {
  const char *s;
  int bytes;
  int cols;
  int start;
  int end;
};

static void row_view_init(struct row_view *v, const struct entity *e, const char *srow) {
  v->s = srow;
  v->bytes = (int)strlen(srow);
  v->cols = utf8_col_width(srow);
  v->start = 0;
  v->end = v->bytes;
  if (!e->trim_edges) return;
  while (v->start < v->bytes && srow[v->start] == ' ') v->start++;
  while (v->end > v->start && srow[v->end - 1] == ' ') v->end--;
}

static bool row_view_transparent(const struct row_view *v, const struct entity *e, int col) {
  int off = v->bytes == v->cols ? col : utf8_byte_offset(v->s, col);
  char ch = v->s[off];
  if (e->sentinel != 0 && ch == e->sentinel) return true;
  return e->trim_edges && ch == ' ' && (off < v->start || off >= v->end);
}

bool entity_glyph_overlap(struct entity *a, struct entity *b) {
  ascii_rows arows = entity_shape(a);
  ascii_rows brows = entity_shape(b);
  if (arows == NULL || brows == NULL) return false;
  int aw = entity_width(a);
  int ah = entity_height(a);
  int bw = entity_width(b);
  int bh = entity_height(b);
  int ax = round_to_int(a->x);
  int ay = round_to_int(a->y);
  int bx = round_to_int(b->x);
  int by = round_to_int(b->y);
  int x0 = ax > bx ? ax : bx;
  int x1 = (ax + aw) < (bx + bw) ? (ax + aw) : (bx + bw);
  int y0 = ay > by ? ay : by;
  int y1 = (ay + ah) < (by + bh) ? (ay + ah) : (by + bh);
  for (int wy = y0; wy < y1; wy++) {
    int arow = wy - ay;
    int brow = wy - by;
    if (arow < 0 || arow >= ah || brow < 0 || brow >= bh) continue;
    if (arows[arow] == NULL || brows[brow] == NULL) continue;
    struct row_view av;
    struct row_view bv;
    row_view_init(&av, a, arows[arow]);
    row_view_init(&bv, b, brows[brow]);
    for (int wx = x0; wx < x1; wx++) {
      int acol = wx - ax;
      int bcol = wx - bx;
      if (acol < 0 || acol >= av.cols || bcol < 0 || bcol >= bv.cols) continue;
      if (row_view_transparent(&av, a, acol)) continue;
      if (row_view_transparent(&bv, b, bcol)) continue;
      return true;
    }
  }
  return false;
}
