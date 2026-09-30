#include "priv.h"
#include "color.h"
#include "xalloc.h"

#include <stdlib.h>
#include <string.h>

static int *g_order_buf = NULL;
static int g_order_cap = 0;
static int *g_bucket_off = NULL;
static int g_bucket_cap = 0;

void entity_draw_shutdown(void) {
  free(g_order_buf);
  g_order_buf = NULL;
  g_order_cap = 0;
  free(g_bucket_off);
  g_bucket_off = NULL;
  g_bucket_cap = 0;
}

static void sort_draw_order(const struct entity_list *list, int *order) {
  int n = list->count;
  int min_z = list->items[0].z;
  int max_z = min_z;
  for (int i = 1; i < n; i++) {
    int z = list->items[i].z;
    if (z < min_z) min_z = z;
    if (z > max_z) max_z = z;
  }
  int range = max_z - min_z + 1;
  if (range > g_bucket_cap) {
    g_bucket_off = xrealloc(g_bucket_off, (size_t)range * sizeof(*g_bucket_off));
    g_bucket_cap = range;
  }
  memset(g_bucket_off, 0, (size_t)range * sizeof(*g_bucket_off));
  for (int i = 0; i < n; i++) g_bucket_off[max_z - list->items[i].z]++;
  int running = 0;
  for (int b = 0; b < range; b++) {
    int count = g_bucket_off[b];
    g_bucket_off[b] = running;
    running += count;
  }
  for (int i = 0; i < n; i++) {
    int b = max_z - list->items[i].z;
    order[g_bucket_off[b]++] = i;
  }
}

static int band_newest_col(const struct entity *e, const char *srow) {
  int len = (int)strlen(srow);
  int cols = utf8_col_width(srow);
  int l = e->hide_x0;
  int r = e->hide_x1 - 1;
  for (; l <= r; l++, r--) {
    int pair[2] = {l, r};
    for (int k = 0; k < (l == r ? 1 : 2); k++) {
      int col = pair[k];
      if (col >= cols) continue;
      int off = utf8_byte_offset(srow, col);
      if (strchr(" _`'", srow[off]) != NULL) continue;
      if (!cell_transparent(e, srow, len, off)) return col;
    }
  }
  return -1;
}

static void draw_turn_seam(struct canvas *c, const struct entity *e, ascii_rows rows, ascii_rows mrows, int base_x, int base_y) {
  int mask_height = 0;
  if (mrows != NULL) while (mrows[mask_height] != NULL) mask_height++;
  int seam = e->hide_x0 + (e->hide_x1 - e->hide_x0) / 2;
  for (int row = 0; rows[row] != NULL; row++) {
    int col = band_newest_col(e, rows[row]);
    if (col < 0) continue;
    struct attr a = e->default_attr;
    if (row < mask_height && col < (int)strlen(mrows[row]) && mrows[row][col] != ' ') a = color_from_mask_letter(mrows[row][col]);
    canvas_put(c, base_x + seam, base_y + row, "|", 1, a, 1);
  }
}

void entity_draw_all(const struct entity_list *list, struct canvas *c, double alpha) {
  int n = list->count;
  if (n <= 0) return;
  if (n > g_order_cap) {
    g_order_buf = xrealloc(g_order_buf, (size_t)n * sizeof(*g_order_buf));
    g_order_cap = n;
  }
  sort_draw_order(list, g_order_buf);
  for (int oi = 0; oi < n; oi++) {
    const struct entity *e = &list->items[g_order_buf[oi]];
    ascii_rows rows = entity_shape(e);
    if (rows == NULL) continue;
    ascii_rows mrows = entity_mask(e);
    int mask_height = 0;
    if (mrows != NULL) while (mrows[mask_height] != NULL) mask_height++;
    double draw_x = e->has_prev ? e->prev_x + (e->x - e->prev_x) * alpha : e->x;
    double draw_y = e->has_prev ? e->prev_y + (e->y - e->prev_y) * alpha : e->y;
    int base_x = round_to_int(draw_x);
    int base_y = round_to_int(draw_y);
    for (int row = 0; rows[row] != NULL; row++) {
      const char *srow = rows[row];
      int byte_len = (int)strlen(srow);
      const char *mrow = (mrows != NULL && row < mask_height) ? mrows[row] : NULL;
      int mlen = mrow != NULL ? (int)strlen(mrow) : 0;
      /* trim bounds per row, not per column */
      int trim_start = 0;
      int trim_end = byte_len;
      if (e->trim_edges) {
        while (trim_start < byte_len && srow[trim_start] == ' ') trim_start++;
        trim_end = byte_len;
        while (trim_end > trim_start && srow[trim_end - 1] == ' ') trim_end--;
      }
      int col = 0;
      for (int byte_idx = 0; byte_idx < byte_len; ) {
        unsigned char ch = (unsigned char)srow[byte_idx];
        int seq_len;
        int width;
        if (ch < 0x80) {
          seq_len = 1;
          width = 1;
        } else {
          seq_len = utf8_seq_len(ch);
          if (byte_idx + seq_len > byte_len) seq_len = byte_len - byte_idx;
          width = utf8_char_width(srow + byte_idx, seq_len);
        }
        bool in_band = e->hidden && col >= e->hide_x0 && col < e->hide_x1;
        int dx = 0;
        if (e->hidden && !in_band) {
          int band = e->hide_x1 - e->hide_x0;
          dx = col < e->hide_x0 ? band / 2 : band / 2 - band;
        }
        bool transparent = (e->sentinel != 0 && (char)ch == e->sentinel) || (e->trim_edges && ch == ' ' && (byte_idx < trim_start || byte_idx >= trim_end));
        if (!in_band && !transparent) {
          struct attr a = e->default_attr;
          if (mrow != NULL && col < mlen && mrow[col] != ' ') a = color_from_mask_letter(mrow[col]);
          canvas_put(c, base_x + col + dx, base_y + row, srow + byte_idx, seq_len, a, width);
        }
        byte_idx += seq_len;
        col += width;
      }
    }
    if (e->hidden) draw_turn_seam(c, e, rows, mrows, base_x, base_y);
  }
}
