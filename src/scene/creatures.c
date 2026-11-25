#include "scene_internal.h"
#include "color.h"
#include "rng.h"
#include "xalloc.h"

#include "art/bigfish.h"
#include "art/classic.h"
#include "art/crab.h"
#include "art/dolphins.h"
#include "art/ducks.h"
#include "art/fish.h"
#include "art/jellyfish.h"
#include "art/misc.h"
#include "art/monster.h"
#include "art/rowers.h"
#include "art/sailboat.h"
#include "art/seahorse.h"
#include "art/shark.h"
#include "art/ship.h"
#include "art/submarine.h"
#include "art/swan.h"
#include "art/swordfish.h"
#include "art/whale.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

int random_swim_y(int h, int sprite_height) {
  int max_height = MAIN_REGION_TOP_ROW;
  int min_height = h - sprite_height;
  return rng_int(min_height - max_height) + max_height;
}

double entry_x(struct entity *e, int w, int dir, int right_inset) {
  return dir ? (double)(w - right_inset) : (double)(1 - entity_width(e));
}

void swim_place(struct entity *e, int w, int h, int dir, int right_inset) {
  e->y = random_swim_y(h, entity_height(e));
  e->x = entry_x(e, w, dir, right_inset);
}

bool swim_fits(int h, ascii_rows shape) {
  int rows = 0;
  while (shape[rows] != NULL) rows++;
  return h - rows >= MAIN_REGION_TOP_ROW;
}

void finish_creature_spawn(struct entity *e, enum entity_type type, int z, double vx, double vy, enum death_action death, struct attr attr) {
  e->type = type;
  e->z = z;
  e->vx = vx;
  e->vy = vy;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = death;
  e->default_attr = attr;
}

/* 4: eye, always white */
static void roll_fish_colors(struct entity *e) {
  color_roll_pick(e->color_pick);
  e->color_pick[4] = 'W';
}

static void apply_fish_colors(struct entity *e, ascii_rows tmpl) {
  char **owned = entity_build_digit_rows(tmpl, e->color_pick);
  entity_clear_owned(e);
  e->owned_mask = owned;
}

static void spawn_fish_from_table(struct scene *sc, const struct sprite_pair *table, int pair_count, int w, int h) {
  int fish_num = rng_int(pair_count);
  if (!swim_fits(h, table[fish_num].shape)) return;
  bool odd = (fish_num % 2) != 0;
  double speed = rng_double(2.0) + 0.25;
  if (odd) speed = -speed;
  struct entity *e = entity_spawn(&sc->entities);
  e->frames = &table[fish_num];
  e->frame_count = 1;
  e->turn_frames = &table[fish_num ^ 1];
  roll_fish_colors(e);
  if (table[fish_num].mask != NULL) apply_fish_colors(e, table[fish_num].mask);
  swim_place(e, w, h, odd, 2);
  e->physical = true;
  finish_creature_spawn(e, ENT_FISH, rng_int(Z_FISH_RANGE) + Z_FISH_MIN, speed, 0, DEATH_ADD_FISH, (struct attr){.col = COL_DEFAULT, .bold = false});
}

#define TURN_STEP_COLS 4

static int turn_max_band(int w) {
  return w > 2 ? w - 2 : 0;
}

static void set_turn_hide(struct entity *e) {
  int w = entity_width(e);
  int band = 2 * e->turn_step;
  if (band > turn_max_band(w)) band = turn_max_band(w);
  e->hidden = true;
  e->hide_x0 = (w - band) / 2;
  e->hide_x1 = e->hide_x0 + band;
}

static void finish_turn(struct entity *e) {
  e->turn_state = TURN_NONE;
  e->hidden = false;
}

#define FISH_EASE_TICKS 3

static int ticks_for(int cols) {
  int n = (cols + TURN_STEP_COLS - 1) / TURN_STEP_COLS;
  return n < FISH_EASE_TICKS ? FISH_EASE_TICKS : n;
}

static void ease_to(struct entity *e, double goal, int ticks) {
  double d = goal - e->vx;
  e->vx_goal = goal;
  e->vx_step = (d < 0.0 ? -d : d) / ticks;
}

void fish_set_vx(const struct scene *sc, struct entity *e, double goal) {
  if (sc->classic_ver != 0) {
    e->vx = goal;
    e->vx_step = 0.0;
    return;
  }
  ease_to(e, goal, FISH_EASE_TICKS);
}

void fish_begin_turn(struct entity *e, double new_vx) {
  e->turn_state = TURN_SHRINK;
  e->turn_step = 0;
  e->turn_vx = new_vx;
  ease_to(e, 0.0, ticks_for((turn_max_band(entity_width(e)) + 1) / 2));
}

static void swap_turn_frames(struct entity *e) {
  const struct sprite_pair *old = e->frames;
  int old_w = entity_width(e);
  e->frames = e->turn_frames;
  e->turn_frames = old;
  entity_shape_changed(e);
  if (e->frames->mask != NULL) apply_fish_colors(e, e->frames->mask);
  int new_w = entity_width(e);
  double shift = (old_w - new_w) / 2.0;
  e->x += shift;
  e->prev_x += shift;
  e->turn_step = (turn_max_band(new_w) + 1) / 2;
  e->turn_state = TURN_GROW;
  e->vx = 0.0;
  ease_to(e, e->turn_vx, ticks_for(e->turn_step));
}

void fish_turn_tick(struct entity *e, int term_w, int uturn_one_in) {
  if (e->turn_state != TURN_NONE) {
    if (e->vy != 0.0) {
      finish_turn(e);
      return;
    }
    if (e->turn_state == TURN_SHRINK) {
      e->turn_step += TURN_STEP_COLS;
      if (2 * e->turn_step >= turn_max_band(entity_width(e)) && e->vx_step == 0.0) swap_turn_frames(e);
    } else {
      e->turn_step -= TURN_STEP_COLS;
      if (e->turn_step <= 0) {
        e->turn_step = 0;
        if (e->vx_step == 0.0) {
          finish_turn(e);
          return;
        }
      }
    }
    set_turn_hide(e);
    return;
  }
  if (e->turn_frames == NULL || e->vy != 0.0 || e->vx == 0.0 || e->feed_heading) return;
  int w = entity_width(e);
  if (e->x < 0.0 || e->x + w > term_w) return;
  if (uturn_one_in <= 0 || rng_int(uturn_one_in) != 0) return;
  fish_begin_turn(e, -e->vx);
}

void spawn_fish(struct scene *sc, int w, int h) {
  bool use_new = sc->classic_ver != 1 && rng_int(12) > 8;
  if (use_new) spawn_fish_from_table(sc, fish_new, 8, w, h);
  else spawn_fish_from_table(sc, fish_old, 16, w, h);
}

/* random-masked pool swimmer */
struct swimmer_def {
  enum entity_type type;
  int z;
  double speed;
  double speed_rand; /* > 0: + rng_double(speed_rand) */
  int frame_count;
  double frame_interval;
  int right_inset;
  const char *color;
  const struct sprite_pair *frames[2];
};

static void spawn_swimmer(struct scene *sc, int w, int h, const struct swimmer_def *def) {
  int dir = rng_int(2);
  const struct sprite_pair *frames = def->frames[dir];
  if (!swim_fits(h, frames[0].shape)) {
    schedule_random_object_return(sc);
    return;
  }
  double speed = def->speed_rand > 0.0 ? rng_double(def->speed_rand) + def->speed : def->speed;
  struct entity *e = entity_spawn(&sc->entities);
  e->frames = frames;
  e->frame_count = def->frame_count;
  e->frame_interval = def->frame_interval;
  entity_randomize_mask(e, frames[0].mask);
  swim_place(e, w, h, dir, def->right_inset);
  finish_creature_spawn(e, def->type, def->z, mirror_speed(dir, speed), 0, DEATH_RANDOM_OBJECT, color_from_name(def->color));
}

static void spawn_big_fish(struct scene *sc, int w, int h) {
  static const struct swimmer_def big1 = {ENT_BIGFISH, Z_BIGFISH, 3.0, 0.0, 1, 0.0, 1, "YELLOW", {&bigfish1[0], &bigfish1[1]}};
  static const struct swimmer_def big2 = {ENT_BIGFISH, Z_BIGFISH, 2.5, 0.0, 1, 0.0, 1, "YELLOW", {&bigfish2[0], &bigfish2[1]}};
  bool use_2 = sc->classic_ver != 1 && rng_int(3) > 1;
  spawn_swimmer(sc, w, h, use_2 ? &big2 : &big1);
}

static void spawn_shark(struct scene *sc, int w, int h) {
  int dir = rng_int(2);
  double x;
  double tx;
  double speed = 2.0;
  bool classic = sc->classic_ver != 0;
  const struct sprite_pair *frames = classic ? &classic_shark[dir] : &shark[dir];
  int rows = 0;
  while (frames->shape[rows]) rows++;
  int lowest = h - rows;
  int y = MAIN_REGION_TOP_ROW + rng_int(lowest - MAIN_REGION_TOP_ROW + 1);
  if (y > lowest) y = lowest < 0 ? 0 : lowest;
  int ty = y + 6;
  if (dir) {
    speed = -2.0;
    x = w - 2;
    tx = x + (classic ? CLASSIC_SHARK_TEETH_COL_LEFT : 6);
  } else {
    x = -53;
    tx = x + (classic ? CLASSIC_SHARK_TEETH_COL_RIGHT : 32);
  }
  if (classic) ty = y + CLASSIC_SHARK_TEETH_ROW;

  struct entity *teeth = entity_spawn(&sc->entities);
  teeth->type = ENT_TEETH;
  teeth->x = tx;
  teeth->y = ty;
  teeth->z = Z_TEETH;
  teeth->vx = speed;
  teeth->vy = 0;
  teeth->frames = teeth_frame;
  teeth->frame_count = 1;
  teeth->physical = true;
  struct entity *e = entity_spawn(&sc->entities);
  e->x = x;
  e->y = y;
  e->frames = frames;
  e->frame_count = 1;
  finish_creature_spawn(e, ENT_SHARK, Z_SHARK, speed, 0, DEATH_RANDOM_OBJECT, color_from_name("CYAN"));
}

struct simple_creature_def {
  enum entity_type type;
  int z;
  double speed;
  double y;
  int frame_count;
  double frame_interval;
  const char *color;
  const struct sprite_pair *frames[2];
};

/* owned per-frame shape + mask copies, digits mapped by pick */
static void own_digit_frames(struct entity *e, const struct sprite_pair *src, int count, double interval, const char pick[10]) {
  char ***shapes = xmalloc((size_t)count * sizeof(*shapes));
  char ***masks = xmalloc((size_t)count * sizeof(*masks));
  for (int i = 0; i < count; i++) {
    shapes[i] = entity_build_digit_rows(src[i].shape, pick);
    masks[i] = entity_build_digit_rows(src[i].mask, pick);
  }
  entity_set_owned_shape_frames(e, shapes, count, interval);
  entity_set_owned_shape_frame_masks(e, masks);
}

static struct entity *spawn_simple_creature(struct scene *sc, int w, const struct simple_creature_def *def) {
  int dir = rng_int(2);
  double speed = mirror_speed(dir, def->speed);
  struct entity *e = entity_spawn(&sc->entities);
  e->frames = def->frames[dir];
  e->frame_count = def->frame_count;
  e->frame_interval = def->frame_interval;
  e->y = def->y;
  e->x = entry_x(e, w, dir, 2);
  finish_creature_spawn(e, def->type, def->z, speed, 0, DEATH_RANDOM_OBJECT, color_from_name(def->color));
  return e;
}

static const struct simple_creature_def ship_def = {ENT_SHIP, Z_SHIP, 1.0, 0, 1, 0.0, "WHITE", {&ship[0], &ship[1]}};
static const struct simple_creature_def whale_def = {ENT_WHALE, Z_WHALE, 1.0, 0, 12, 10.0, "WHITE", {whale[0], whale[1]}};
static const struct simple_creature_def submarine_def = {ENT_SUBMARINE, Z_SUBMARINE, 1.0, 6, 9, 0.0, "YELLOW", {submarine[0], submarine[1]}};
static const struct simple_creature_def ducks_def = {ENT_DUCK, Z_DUCK, 1.0, 5, 3, 2.5, "WHITE", {ducks[0], ducks[1]}};

static void spawn_sailboat(struct scene *sc, int w, int h) {
  (void)h;
  static const char sail_letters[] = {'r', 'R', 'g', 'G', 'w', 'm', 'M', 'Y', 'B'};
  static const struct simple_creature_def def = {ENT_SAILBOAT, Z_SAILBOAT, 1.0, 0, 1, 0.0, "WHITE", {&sailboat[0], &sailboat[1]}};
  struct entity *e = spawn_simple_creature(sc, w, &def);
  const char pick[10] = {[1] = sail_letters[rng_int((int)sizeof(sail_letters))]};
  e->owned_mask = entity_build_digit_rows(e->frames->mask, pick);
  e->y = (double)(1 + rng_int(3));
}

static void spawn_monster(struct scene *sc, int w, int h) {
  (void)h;
  static const struct simple_creature_def new_def = {ENT_MONSTER, Z_MONSTER, 2.0, 2, 2, 2.5, "GREEN", {monster_new[0], monster_new[1]}};
  static const struct simple_creature_def old_def = {ENT_MONSTER, Z_MONSTER, 2.0, 2, 4, 2.5, "GREEN", {monster_old[0], monster_old[1]}};
  spawn_simple_creature(sc, w, sc->classic_ver != 1 ? &new_def : &old_def);
}

static void spawn_swordfish(struct scene *sc, int w, int h) {
  static const struct swimmer_def def = {ENT_SWORDFISH, Z_SWORDFISH, 3.5, 0.0, 1, 0.0, 1, "YELLOW", {&swordfish[0], &swordfish[1]}};
  spawn_swimmer(sc, w, h, &def);
}

static void spawn_swan(struct scene *sc, int w, int h) {
  (void)h;
  static const struct simple_creature_def def = {ENT_SWAN, Z_SWAN, 1.0, 1, 1, 0.0, "WHITE", {&swan[0], &swan[1]}};
  struct entity *parent = spawn_simple_creature(sc, w, &def);
  if (rng_int(5) != 0) return;
  int parent_id = parent->id;
  int parent_w = entity_width(parent);
  int parent_bottom = (int)parent->y + entity_height(parent);
  double px = parent->x;
  double vx = parent->vx;
  struct entity *baby = entity_spawn(&sc->entities);
  baby->frames = &swan_baby[vx < 0.0];
  baby->frame_count = 1;
  int bw = entity_width(baby);
  baby->x = vx > 0.0 ? px - 1 - bw : px + parent_w + 1;
  baby->y = parent_bottom - entity_height(baby);
  baby->following = true;
  baby->follow_id = parent_id;
  baby->follow_vx = vx;
  finish_creature_spawn(baby, ENT_SWAN_BABY, Z_SWAN, vx, 0, DEATH_NONE, color_from_name("WHITE"));
}

static void spawn_rowers(struct scene *sc, int w, int h) {
  (void)h;
  static const char boat_letters[] = {'r', 'R', 'g', 'G', 'm', 'M', 'y', 'Y', 'B'};
  static const struct simple_creature_def defs[2] = {
    {ENT_ROWERS, Z_ROWERS, 1.0, 4, 5, 2.5, "WHITE", {rowers[0], rowers[1]}},
    {ENT_ROWERS, Z_ROWERS, 1.0, 4, 5, 2.5, "WHITE", {rowers_short[0], rowers_short[1]}},
  };
  const struct simple_creature_def def = defs[rng_int(2)];
  struct entity *e = spawn_simple_creature(sc, w, &def);
  const char pick[10] = {[1] = boat_letters[rng_int((int)sizeof(boat_letters))]};
  own_digit_frames(e, e->frames, def.frame_count, def.frame_interval, pick);
}

static void spawn_dolphins(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double speed = mirror_speed(dir, 1.0);
  double distance = mirror_speed(dir, 15.0);
  double x = dir ? (double)(w - 2) : -13.0;
  static const struct {
    double distance_mult;
    double y;
    int age_ticks;
    const char *color;
    enum death_action death;
  } pod[3] = {
      {2.0, 8.0, 0, "blue", DEATH_NONE},
      {1.0, 2.0, 12, "BLUE", DEATH_NONE},
      {0.0, 5.0, 24, "CYAN", DEATH_RANDOM_OBJECT},
  };

  for (int i = 0; i < 3; i++) {
    struct entity *e = entity_spawn(&sc->entities);
    e->x = x - distance * pod[i].distance_mult;
    e->y = pod[i].y;
    e->age_ticks = pod[i].age_ticks;
    e->frames = dolphins[dir];
    e->frame_count = 2;
    e->frame_interval = 5.0;
    finish_creature_spawn(e, ENT_DOLPHIN, Z_DOLPHIN, speed, 0, pod[i].death, color_from_name(pod[i].color));
  }
}

static void spawn_crab(struct scene *sc, int w, int h) {
  static const char *const colors[4] = {"red", "RED", "yellow", "YELLOW"};
  int dir = rng_int(2);
  double speed = mirror_speed(dir, rng_double(0.4) + 0.15);
  struct entity *e = entity_spawn(&sc->entities);
  e->frames = crab_frames;
  e->frame_count = 2;
  e->frame_interval = 3.0;
  e->y = h - entity_height(e);
  e->x = entry_x(e, w, dir, 2);
  finish_creature_spawn(e, ENT_CRAB, rng_int(Z_FLORA_RANGE) + Z_FISH_MIN, speed, 0, DEATH_RANDOM_OBJECT, color_from_name(colors[rng_int(4)]));
}

void spawn_jellyfish(struct scene *sc, int w, int h) {
  if (!sc->aquatic.jellyfish) return;
  int dir = rng_int(2);
  double speed = mirror_speed(dir, rng_double(0.3) + 0.1);
  static const char body_letters[] = {'c', 'b', 'K'};
  const char pick[10] = {[1] = body_letters[rng_int((int)sizeof(body_letters))]};
  const struct sprite_pair *src = rng_int(2) ? jellyfish_small_frames : jellyfish_frames;
  if (!swim_fits(h, src[0].shape)) return;
  struct entity *e = entity_spawn(&sc->entities);
  own_digit_frames(e, src, 2, 6.0, pick);
  swim_place(e, w, h, dir, 2);
  finish_creature_spawn(e, ENT_JELLYFISH, Z_JELLYFISH, speed, 0, DEATH_NONE, color_from_name("cyan"));
}

static void spawn_seahorse(struct scene *sc, int w, int h) {
  static const struct swimmer_def def = {ENT_SEAHORSE, Z_SEAHORSE, 0.5, 0.4, 2, 4.0, 2, "YELLOW", {seahorse[0], seahorse[1]}};
  spawn_swimmer(sc, w, h, &def);
}

typedef void (*spawn_fn)(struct scene *, int, int);

/* spawn fn or simple def */
struct pool_species {
  size_t flag;
  enum entity_type type;
  spawn_fn spawn;
  const struct simple_creature_def *simple;
};

#define POOL(flag, fn, type) {offsetof(struct aquatic_life, flag), type, fn, NULL}
#define POOL_SIMPLE(flag, def, type) {offsetof(struct aquatic_life, flag), type, NULL, &(def)}

/* order = pick order */
static const struct pool_species random_pool[] = {
  POOL_SIMPLE(ship, ship_def, ENT_SHIP),           POOL_SIMPLE(whale, whale_def, ENT_WHALE),
  POOL(monster, spawn_monster, ENT_MONSTER),       POOL(bigfish, spawn_big_fish, ENT_BIGFISH),
  POOL(shark, spawn_shark, ENT_SHARK),             POOL_SIMPLE(submarine, submarine_def, ENT_SUBMARINE),
  POOL(swordfish, spawn_swordfish, ENT_SWORDFISH), POOL_SIMPLE(ducks, ducks_def, ENT_DUCK),
  POOL(dolphins, spawn_dolphins, ENT_DOLPHIN),     POOL(swan, spawn_swan, ENT_SWAN),
  POOL(fishhook, spawn_fishhook, ENT_FISHHOOK),    POOL(crab, spawn_crab, ENT_CRAB),
  POOL(seahorse, spawn_seahorse, ENT_SEAHORSE),    POOL(rowers, spawn_rowers, ENT_ROWERS),
  POOL(sailboat, spawn_sailboat, ENT_SAILBOAT),
};
#define RANDOM_POOL_COUNT (sizeof(random_pool) / sizeof(random_pool[0]))

bool random_pool_enabled(const struct aquatic_life *a) {
  for (size_t i = 0; i < RANDOM_POOL_COUNT; i++) if (aquatic_on(a, random_pool[i].flag)) return true;
  return false;
}

bool random_pool_has_type(enum entity_type type) {
  for (size_t i = 0; i < RANDOM_POOL_COUNT; i++) if (random_pool[i].type == type) return true;
  return false;
}

void spawn_random_object(struct scene *sc, int w, int h) {
  static const struct pool_species message_event = {0, ENT_MESSAGE, spawn_message_event, NULL};
  const struct pool_species *enabled[RANDOM_POOL_COUNT + 1];
  int enabled_count = 0;
  for (size_t i = 0; i < RANDOM_POOL_COUNT; i++) if (aquatic_on(&sc->aquatic, random_pool[i].flag)) enabled[enabled_count++] = &random_pool[i];
  if (sc->message_rows != NULL && sc->classic_ver == 0 && sc->message_position == MSG_POS_EVENT) enabled[enabled_count++] = &message_event;
  if (enabled_count == 0) return;
  const struct pool_species *p = enabled[rng_int(enabled_count)];
  if (p->simple != NULL) spawn_simple_creature(sc, w, p->simple);
  else p->spawn(sc, w, h);
}

void schedule_random_object_return(struct scene *sc) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_RANDOM_OBJECT_TIMER;
  e->die_after = DIE_SECONDS(rng_double(12.0) + 3.0);
  e->death_action = DEATH_ADD_RANDOM_OBJECT;
}
