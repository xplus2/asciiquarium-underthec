#ifndef UNDERTHEC_ENTITY_PRIV_H
#define UNDERTHEC_ENTITY_PRIV_H

#include "entity.h"
#include "../utf8.h"

/* [start, end): B inside leading/trailing spaces */
static inline void row_trim(const char *s, int len, int *start, int *end) {
  int a = 0;
  while (a < len && s[a] == ' ') a++;
  int b = len;
  while (b > a && s[b - 1] == ' ') b--;
  *start = a;
  *end = b;
}

/* shape row with trim bounds, built once per row */
struct row_view {
  const char *s;
  int bytes;
  int cols;
  int start;
  int end;
};

void row_view_init(struct row_view *v, const struct entity *e, const char *srow);
int row_view_offset(const struct row_view *v, int col);
/* off: B offset */
bool row_view_transparent_at(const struct row_view *v, const struct entity *e, int off);

void tick_seaweed_growth(struct entity *e);
void tick_seaweed_debris(struct entity *e, int term_h);

#endif
