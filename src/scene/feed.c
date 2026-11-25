#include "scene_internal.h"
#include "color.h"
#include "rng.h"
#include "xalloc.h"
#include <math.h>
#include <string.h>

#define FLAKE_COUNT 10
#define FLAKE_BAND_WIDTH 4
#define FLAKE_FALL_SPEED 2.0
#define FLAKE_TRICKLE_BASE 0.1
#define FLAKE_MAIN_BASE 0.5
#define FLAKE_CHAR_INTERVAL 3.0
#define FLAKE_BOTTOM_LINGER 3.0
#define FLAKE_SPEED_MULT_BASE 0.8
#define FLAKE_SPEED_MULT_RANGE 0.4
#define FEED_HEADING_SPEEDUP 1.5

static const char flake_chars[] = ",'`_-";

static char flake_pick_char(void) {
  return flake_chars[rng_int((int)sizeof(flake_chars) - 1)];
}

static void spawn_flake(struct scene *sc, int x) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_FLAKE;
  e->x = x;
  e->y = 0.0;
  e->z = Z_FLAKE;
  e->feed_speed_mult = FLAKE_SPEED_MULT_BASE + rng_double(FLAKE_SPEED_MULT_RANGE);
  e->default_attr = color_from_name("yellow");
  char *row = xmalloc(2);
  row[0] = flake_pick_char();
  row[1] = '\0';
  entity_set_owned_single_row(e, row, 0.0);
}

void feed_trigger(struct scene *sc, int w, int h, int col) {
  (void)h;
  bool have_fish = false;
  double lowest_fish_y = 0.0;
  for (int i = 0; i < sc->entities.count; i++) {
    const struct entity *e = &sc->entities.items[i];
    if (e->marked_dead || e->type != ENT_FISH) continue;
    if (!have_fish || e->y > lowest_fish_y) {
      lowest_fish_y = e->y;
      have_fish = true;
    }
  }
  for (int i = 0; i < sc->entities.count; i++) {
    const struct entity *e = &sc->entities.items[i];
    if (e->marked_dead || e->type != ENT_FLAKE) continue;
    if (have_fish ? e->y <= lowest_fish_y : e->die_after < 0) return;
  }
  int max_band = w - FLAKE_BAND_WIDTH;
  if (max_band < 1) max_band = 1;
  int band_x0;
  if (col == FEED_COL_AUTO) band_x0 = rng_int(max_band);
  else {
    band_x0 = col - FLAKE_BAND_WIDTH / 2;
    if (band_x0 < 0) band_x0 = 0;
    if (band_x0 > max_band) band_x0 = max_band;
  }
  for (int i = 0; i < FLAKE_COUNT; i++) spawn_flake(sc, band_x0 + rng_int(FLAKE_BAND_WIDTH));
  sc->feed_alerted = false;
  sc->feed_active = true;
}

static void feed_tick_flakes(struct scene *sc, int floor_row) {
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *e = &sc->entities.items[i];
    if (e->marked_dead || e->type != ENT_FLAKE) continue;
    if (e->die_after >= 0) continue;
    double old_y = e->y;
    double vy;
    if (old_y < WATER_SURFACE_ROW) vy = FLAKE_FALL_SPEED;
    else if (old_y < MAIN_REGION_TOP_ROW) vy = FLAKE_TRICKLE_BASE * e->feed_speed_mult;
    else vy = FLAKE_MAIN_BASE * e->feed_speed_mult;
    e->y = old_y + vy;
    if (e->y > floor_row) e->y = floor_row;
    bool crossed_surface = old_y < WATER_SURFACE_ROW && e->y >= WATER_SURFACE_ROW;
    if (crossed_surface) e->y = WATER_SURFACE_ROW;
    bool at_floor = e->y >= floor_row;
    if (at_floor) {
      entity_set_owned_lead(e, '_');
      e->die_after = DIE_SECONDS(FLAKE_BOTTOM_LINGER);
    } else if (crossed_surface) {
      entity_set_owned_lead(e, '_');
      e->frame_timer = 0.0;
    } else {
      e->frame_timer += 1.0;
      if (e->frame_timer >= FLAKE_CHAR_INTERVAL) {
        e->frame_timer -= FLAKE_CHAR_INTERVAL;
        entity_set_owned_lead(e, flake_pick_char());
      }
    }
  }
}

/* scratch: flake columns, then flake indices. sized to entity count */
static int *g_cols_buf = NULL;
static int g_cols_cap = 0;

void feed_shutdown(void) {
  free(g_cols_buf);
  g_cols_buf = NULL;
  g_cols_cap = 0;
}

static int *scratch(const struct scene *sc) {
  if (sc->entities.count > g_cols_cap) {
    g_cols_buf = xrealloc(g_cols_buf, (size_t)sc->entities.count * sizeof(*g_cols_buf));
    g_cols_cap = sc->entities.count;
  }
  return g_cols_buf;
}

static void feed_alert_fish(struct scene *sc) {
  if (sc->feed_alerted) return;
  int *cols = scratch(sc);
  int col_count = 0;
  bool any_in_main = false;
  for (int i = 0; i < sc->entities.count; i++) {
    const struct entity *e = &sc->entities.items[i];
    if (e->marked_dead || e->type != ENT_FLAKE || e->die_after >= 0) continue;
    if (e->y >= MAIN_REGION_TOP_ROW) any_in_main = true;
    int col = round_to_int(e->x);
    bool dup = false;
    for (int k = 0; k < col_count; k++) if (cols[k] == col) { dup = true; break; }
    if (!dup) cols[col_count++] = col;
  }
  if (!any_in_main || col_count == 0) return;

  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *fish = &sc->entities.items[i];
    if (fish->marked_dead || fish->type != ENT_FISH) continue;
    if (fish->vy != 0.0 || fish->turn_state != TURN_NONE || fish->feed_heading) continue;
    int col = cols[rng_int(col_count)];
    int width = entity_width(fish);
    double front = fish->vx > 0 ? fish->x + width : fish->x;
    bool away = (fish->vx > 0 && (double)col < front) || (fish->vx < 0 && (double)col > front);
    double cruise = fish->vx_step > 0.0 ? fish->vx_goal : fish->vx;
    double sped = fabs(cruise) * FEED_HEADING_SPEEDUP;
    fish->feed_target_col = col;
    fish->feed_heading = true;
    fish->feed_resume_vx = away ? -cruise : cruise;
    if (away) fish_begin_turn(fish, cruise > 0.0 ? -sped : sped);
    else fish_set_vx(sc, fish, cruise > 0.0 ? sped : -sped);
  }
  sc->feed_alerted = true;
}

static void feed_advance_heading_fish(struct scene *sc) {
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *fish = &sc->entities.items[i];
    if (fish->marked_dead || fish->type != ENT_FISH) continue;
    if (!fish->feed_heading || fish->feed_wait || fish->turn_state != TURN_NONE) continue;
    int width = entity_width(fish);
    double front = fish->vx > 0 ? fish->x + width : fish->x;
    bool reached = (fish->vx > 0 && front >= fish->feed_target_col) || (fish->vx < 0 && front <= fish->feed_target_col);
    if (reached) {
      fish->x = fish->vx > 0 ? fish->feed_target_col - width : fish->feed_target_col;
      fish->has_prev = false;
      fish->vx = 0.0;
      fish->feed_wait = true;
    }
  }
}

static bool shape_col_has_ink(ascii_rows rows, int h, int c) {
  for (int r = 0; r < h; r++) {
    const char *row = rows[r];
    if (row != NULL && c < (int)strlen(row) && row[c] != ' ') return true;
  }
  return false;
}

/* first two inked cols from front. ret count */
static int fish_front_cols(struct entity *fish, int out[2]) {
  ascii_rows rows = entity_shape(fish);
  int w = entity_width(fish);
  int h = entity_height(fish);
  if (rows == NULL || w <= 0 || h <= 0) return 0;
  int found = 0;
  for (int i = 0; i < w && found < 2; i++) {
    int c = fish->vx >= 0.0 ? w - 1 - i : i;
    if (shape_col_has_ink(rows, h, c)) out[found++] = c;
  }
  return found;
}

static bool fish_mouth_touch(struct entity *fish, const int front_cols[2], int found, const struct entity *flake) {
  int flake_row = round_to_int(flake->y) - round_to_int(fish->y);
  if (flake_row < 0 || flake_row >= entity_height(fish)) return false;
  int flake_col = round_to_int(flake->x) - round_to_int(fish->x);
  for (int k = 0; k < found; k++) if (front_cols[k] == flake_col) return true;
  return false;
}

static void feed_check_touches(struct scene *sc) {
  int *flakes = scratch(sc);
  int flake_count = 0;
  for (int j = 0; j < sc->entities.count; j++) {
    const struct entity *e = &sc->entities.items[j];
    if (!e->marked_dead && e->type == ENT_FLAKE) flakes[flake_count++] = j;
  }
  if (flake_count == 0) return;
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *fish = &sc->entities.items[i];
    if (fish->marked_dead || fish->type != ENT_FISH || !fish->feed_heading) continue;
    int front_cols[2];
    int found = fish_front_cols(fish, front_cols);
    if (found == 0) continue;
    for (int k = 0; k < flake_count; k++) {
      struct entity *flake = &sc->entities.items[flakes[k]];
      if (flake->marked_dead) continue;
      if (fish_mouth_touch(fish, front_cols, found, flake)) {
        flake->marked_dead = true;
        fish_set_vx(sc, fish, fish->feed_resume_vx);
        fish->feed_heading = false;
        fish->feed_wait = false;
        break;
      }
    }
  }
}

static void feed_release_waiting_fish(struct scene *sc) {
  bool any_flake = false;
  double top_flake_y = 0.0;
  for (int j = 0; j < sc->entities.count; j++) {
    const struct entity *flake = &sc->entities.items[j];
    if (flake->marked_dead || flake->type != ENT_FLAKE) continue;
    if (!any_flake || flake->y < top_flake_y) top_flake_y = flake->y;
    any_flake = true;
  }
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *fish = &sc->entities.items[i];
    if (fish->marked_dead || fish->type != ENT_FISH || !fish->feed_heading) continue;
    if (!any_flake || top_flake_y > fish->y) {
      fish_set_vx(sc, fish, fish->feed_resume_vx);
      fish->feed_heading = false;
      fish->feed_wait = false;
    }
  }
}

void feed_tick(struct scene *sc, int w, int h, bool flakes) {
  (void)w;
  if (!flakes) {
    feed_release_waiting_fish(sc);
    sc->feed_active = false;
    return;
  }
  int floor_row = h - 1;
  feed_tick_flakes(sc, floor_row);
  feed_alert_fish(sc);
  feed_check_touches(sc);
  feed_advance_heading_fish(sc);
  feed_release_waiting_fish(sc);
}
