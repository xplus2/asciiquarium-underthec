#include "config.h"
#include "color.h"
#include "opts.h"
#include "rng.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void config_init(struct config *cfg) {
  *cfg = (struct config){
      .aquatic = scene_aquatic_default(),
      .no_castle = false,
      .message_position = MSG_POS_MIDDLE,
      .pace = 1.0,
      .uturn_chance = 400,
      .fps = DEFAULT_FPS,
      .colors_mode = 16,
  };
}

void config_free(struct config *cfg) {
  free(cfg->message);
  free(cfg->message_color);
  free(cfg->castle_name);
  free(cfg->time_color);
  cfg->message = NULL;
  cfg->message_color = NULL;
  cfg->castle_name = NULL;
  cfg->time_color = NULL;
}

enum opt_id {
  OPT_CLASSIC, OPT_AQUATIC, OPT_FISH, OPT_MESSAGE, OPT_MESSAGE_COLOR, OPT_MESSAGE_POSITION,
  OPT_TIME, OPT_TIME_COLOR, OPT_CASTLE_NAME, OPT_NO_CASTLE, OPT_PACE, OPT_UTURN, OPT_FPS, OPT_COLORS, OPT_COUNT
};

struct opt_def {
  const char *name;
  const char *env;
  bool classic_off;
};

/* env order: classic first, fish after aquatic-life */
static const struct opt_def opt_defs[OPT_COUNT] = {
  [OPT_CLASSIC]          = {"classic", "UNDERTHEC_CLASSIC", false},
  [OPT_AQUATIC]          = {"aquatic-life", "UNDERTHEC_AQUATIC_LIFE", true},
  [OPT_FISH]             = {"fish", "UNDERTHEC_FISH", true},
  [OPT_MESSAGE]          = {"message", "UNDERTHEC_MESSAGE", true},
  [OPT_MESSAGE_COLOR]    = {"message-color", "UNDERTHEC_MESSAGE_COLOR", true},
  [OPT_MESSAGE_POSITION] = {"message-position", "UNDERTHEC_MESSAGE_POSITION", true},
  [OPT_TIME]             = {"time", "UNDERTHEC_TIME", true},
  [OPT_TIME_COLOR]       = {"time-color", "UNDERTHEC_TIME_COLOR", true},
  [OPT_CASTLE_NAME]      = {"castle-name", "UNDERTHEC_CASTLE_NAME", true},
  [OPT_NO_CASTLE]        = {"no-castle", "UNDERTHEC_NO_CASTLE", false},
  [OPT_PACE]             = {"pace", "UNDERTHEC_PACE", false},
  [OPT_UTURN]            = {"uturn-chance", "UNDERTHEC_UTURN_CHANCE", true},
  [OPT_FPS]              = {"fps", "UNDERTHEC_FPS", true},
  [OPT_COLORS]           = {"colors", "UNDERTHEC_COLORS", false},
};

#define OPT_BIT(id) (1u << (id))

static int opt_find(const char *name) {
  for (int i = 0; i < OPT_COUNT; i++) if (strcmp(name, opt_defs[i].name) == 0) return i;
  return -1;
}

static bool fail(char *err, size_t err_len, const char *msg, const char *shown) {
  opts_set_errbuf(err, err_len, (const char *[]){msg, " for ", shown}, 3);
  return false;
}

bool config_set(struct config *cfg, const char *name, const char *value, const char *shown, char *err, size_t err_len) {
  char eb[128];
  bool ok = true;
  int id = opt_find(name);
  switch (id) {
    case OPT_CLASSIC:
      ok = opts_parse_classic(value, &cfg->classic_ver, eb, sizeof eb);
      break;
    case OPT_AQUATIC: {
      bool fish_set = false;
      ok = opts_parse_aquatic_life(value, &cfg->aquatic, &fish_set, eb, sizeof eb);
      if (ok && fish_set) cfg->set_mask |= OPT_BIT(OPT_FISH);
      break;
    }
    case OPT_FISH:
      ok = opts_parse_fish_count(value, &cfg->aquatic.fish_count, eb, sizeof eb);
      break;
    case OPT_MESSAGE:
      free(cfg->message);
      cfg->message = opts_strdup(value);
      break;
    case OPT_MESSAGE_COLOR:
      ok = color_name_valid(value);
      if (!ok) {
        opts_set_errbuf(eb, sizeof eb, (const char *[]){"invalid color '", value, "'"}, 3);
        break;
      }
      free(cfg->message_color);
      cfg->message_color = opts_strdup(value);
      break;
    case OPT_MESSAGE_POSITION:
      ok = opts_parse_message_position(value, &cfg->message_position, eb, sizeof eb);
      break;
    case OPT_TIME:
      ok = opts_parse_time(value, &cfg->time_mode, eb, sizeof eb);
      break;
    case OPT_TIME_COLOR:
      ok = color_name_valid(value);
      if (!ok) {
        opts_set_errbuf(eb, sizeof eb, (const char *[]){"invalid color '", value, "'"}, 3);
        break;
      }
      free(cfg->time_color);
      cfg->time_color = opts_strdup(value);
      break;
    case OPT_CASTLE_NAME: {
      char name_buf[CASTLE_NAME_MAX_BYTES + 1];
      ok = opts_parse_castle_name(value, name_buf, sizeof name_buf, eb, sizeof eb);
      if (!ok) break;
      free(cfg->castle_name);
      cfg->castle_name = opts_strdup(name_buf);
      break;
    }
    case OPT_NO_CASTLE:
      ok = opts_parse_bool(value, &cfg->no_castle, eb, sizeof eb);
      break;
    case OPT_PACE:
      ok = opts_parse_pace(value, &cfg->pace, eb, sizeof eb);
      break;
    case OPT_UTURN:
      ok = opts_parse_uturn_chance(value, &cfg->uturn_chance, eb, sizeof eb);
      break;
    case OPT_FPS:
      ok = opts_parse_fps(value, &cfg->fps, eb, sizeof eb);
      break;
    case OPT_COLORS:
      ok = opts_parse_colors(value, &cfg->colors_mode, eb, sizeof eb);
      break;
    default:
      return fail(err, err_len, "unknown option", shown);
  }
  if (!ok) return fail(err, err_len, eb, shown);
  cfg->set_mask |= OPT_BIT(id);
  return true;
}

bool config_is_set(const struct config *cfg, const char *name) {
  int id = opt_find(name);
  return id >= 0 && (cfg->set_mask & OPT_BIT(id)) != 0;
}

bool config_apply_env(struct config *cfg, char *err, size_t err_len) {
  unsigned given = cfg->set_mask;
  for (int id = 0; id < OPT_COUNT; id++) {
    const struct opt_def *d = &opt_defs[id];
    if ((given & OPT_BIT(id)) != 0 || (d->classic_off && cfg->classic_ver != 0)) continue;
    const char *value = getenv(d->env);
    if (value != NULL && !config_set(cfg, d->name, value, d->env, err, err_len)) return false;
  }
  return true;
}

bool config_check(struct config *cfg, char *err, size_t err_len) {
  (void)err;
  (void)err_len;
  if (cfg->classic_ver == 0) return true;
  cfg->aquatic = scene_aquatic_classic();
  cfg->fps = CLASSIC_FPS;
  cfg->uturn_chance = 0;
  free(cfg->message);
  cfg->message = NULL;
  free(cfg->castle_name);
  cfg->castle_name = NULL;
  free(cfg->time_color);
  cfg->time_color = NULL;
  cfg->time_mode = 0;
  return true;
}

void config_start(const struct config *cfg, struct app *app, double now) {
  rng_seed((uint64_t)time(NULL) ^ ((uint64_t)clock() << 32));
  app_init(app, cfg->classic_ver, cfg->aquatic, cfg->pace, cfg->fps, cfg->colors_mode, now);
  if (cfg->message_color != NULL) scene_set_message_color(&app->scene, color_from_name(cfg->message_color));
  scene_set_message_position(&app->scene, cfg->message_position);
  if (cfg->time_color != NULL) scene_set_time_color(&app->scene, color_from_name(cfg->time_color));
  scene_set_time(&app->scene, cfg->time_mode);
  scene_set_uturn_chance(&app->scene, cfg->uturn_chance);
  scene_set_castle(&app->scene, !cfg->no_castle);
  if (cfg->castle_name != NULL) scene_set_castle_name(&app->scene, cfg->castle_name);
  if (cfg->message != NULL) {
    char *buf = opts_strdup(cfg->message);
    char **rows = NULL;
    int count = opts_split_lines(buf, &rows);
    if (count > 0) scene_set_message(&app->scene, (const char *const *)rows, count);
    free(rows);
    free(buf);
  }
}
