#include "scene_internal.h"
#include "color.h"
#include "rng.h"

#include "art/turtle.h"
#include <string.h>

static void map_turtle_colors(const char *in, char *out, void *ctx) {
  char shell = *(const char *)ctx;
  size_t len = strlen(in);
  for (size_t j = 0; j < len; j++) out[j] = (in[j] == '2') ? shell : in[j];
  out[len] = '\0';
}

static void randomize_turtle_mask(struct entity *e, ascii_rows tmpl) {
  static const char shell_letters[3] = {'g', 'r', 'R'};
  char shell = shell_letters[rng_int(3)];
  e->owned_mask = entity_build_transformed_rows(tmpl, map_turtle_colors, &shell);
}

static int live_turtle_count(const struct scene *sc) {
  int n = 0;
  for (int i = 0; i < sc->entities.count; i++) {
    const struct entity *e = &sc->entities.items[i];
    if (!e->marked_dead && e->type == ENT_TURTLE) n++;
  }
  return n;
}

void spawn_turtle(struct scene *sc, int w, int h) {
  if (!sc->aquatic.turtle) return;
  if (live_turtle_count(sc) >= 3) return;
  int dir = rng_int(2);
  double speed = mirror_speed(dir, rng_double(0.5) + 0.3);
  struct entity *e = entity_spawn(&sc->entities);
  e->frames = &turtle[dir];
  e->frame_count = 1;
  randomize_turtle_mask(e, turtle[dir].mask);
  int width = entity_width(e);
  int height = entity_height(e);
  e->y = random_swim_y(h, height);
  e->x = dir ? (double)(w - 2) : (double)(1 - width);
  finish_creature_spawn(e, ENT_TURTLE, Z_TURTLE, speed, 0, DEATH_NONE, color_from_name("WHITE"));
}

void schedule_turtle_return(struct scene *sc) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_TURTLE_TIMER;
  e->die_after = rng_double(300.0) + 150.0;
  e->death_action = DEATH_ADD_TURTLE;
}

void turtle_timer_fire(struct scene *sc, int w, int h) {
  spawn_turtle(sc, w, h);
  schedule_turtle_return(sc);
}
