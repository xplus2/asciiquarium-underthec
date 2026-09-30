#include "../../app.h"
#include "../../config.h"
#include "../../xalloc.h"
#include "../../entity/priv.h"

#include <stdint.h>

/* JS calls */
const char *web_opt(const char *name, const char *value, const char *shown);
const char *web_init(double now_ms);
int web_fps(void);
void web_resize(int cols, int rows);
void web_key(int key);
void web_click(int col, int row);
const uint32_t *web_frame(double now_ms);
int web_width(void);
int web_height(void);
bool web_settings_open(void);
bool web_help_open(void);

static struct app app;
static uint32_t *packed;
static size_t packed_cap;
static char err[256];
static bool cfg_ready;
static struct config cfg;

static void cfg_defaults(void) {
  if (cfg_ready) return;
  config_init(&cfg);
  cfg_ready = true;
}

const char *web_opt(const char *name, const char *value, const char *shown) {
  cfg_defaults();
  return config_set(&cfg, name, value, shown, err, sizeof err) ? NULL : err;
}

const char *web_init(double now_ms) {
  cfg_defaults();
  if (!config_check(&cfg, err, sizeof err)) {
    config_free(&cfg);
    return err;
  }
  config_start(&cfg, &app, now_ms / 1000.0);
  config_free(&cfg);
  return NULL;
}

/* settings dialog changes it */
int web_fps(void) { return app.fps; }

void web_resize(int cols, int rows) {
  app_resize(&app, cols, rows);
  size_t n = (size_t)app.canvas.width * (size_t)app.canvas.height;
  if (n > packed_cap) {
    packed = xrealloc(packed, n * sizeof(*packed));
    packed_cap = n;
  }
}

void web_key(int key) { app_key(&app, key); }

void web_click(int col, int row) { app_click(&app, col, row); }

const uint32_t *web_frame(double now_ms) {
  app_frame(&app, now_ms / 1000.0);
  size_t n = (size_t)app.canvas.width * (size_t)app.canvas.height;
  for (size_t i = 0; i < n; i++) {
    const struct cell *c = &app.canvas.cells[i];
    uint32_t cp = c->cont ? 0 : (uint32_t)utf8_decode(c->glyph, utf8_seq_len((unsigned char)c->glyph[0]));
    packed[i] = cp << 10 | (c->bg_bold ? 1u : 0u) << 9 | (uint32_t)c->bg << 5 | (uint32_t)c->col << 1 | (c->bold ? 1u : 0u);
  }
  return packed;
}

int web_width(void) { return app.canvas.width; }

int web_height(void) { return app.canvas.height; }

bool web_settings_open(void) { return settings_ui_is_open(&app.settings); }

bool web_help_open(void) { return help_ui_is_open(&app.help); }
