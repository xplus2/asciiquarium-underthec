#ifndef UNDERTHEC_CONFIG_H
#define UNDERTHEC_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#include "app.h"
#include "scene.h"

/* options by long CLI name */
struct config {
  int classic_ver; /* 0=off, 1=1.0, 2=1.1 */
  struct aquatic_life aquatic;
  char *message;
  char *message_color;
  char *castle_name;
  bool no_castle;
  enum message_position message_position;
  int time_mode; /* 0=off, 12, 24 */
  char *time_color;
  double pace;
  int uturn_chance;
  int fps;
  int colors_mode;
  unsigned set_mask; /* by config_set */
};

void config_init(struct config *cfg);
void config_free(struct config *cfg);

/* name in errors. false: err set */
bool config_set(struct config *cfg, const char *name, const char *value, const char *shown, char *err, size_t err_len);
bool config_is_set(const struct config *cfg, const char *name);
/* UNDERTHEC_* for options not set yet. classic skips disabled opts */
bool config_apply_env(struct config *cfg, char *err, size_t err_len);
/* cross options. classic wins: disabled options reset */
bool config_check(struct config *cfg, char *err, size_t err_len);
/* rng, app init */
void config_start(const struct config *cfg, struct app *app, double now);

#endif
