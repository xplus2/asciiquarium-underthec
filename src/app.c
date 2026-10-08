#include "app.h"

#include <string.h>

static bool too_small(const struct app *a) { return a->h < APP_MIN_ROWS; }

static void draw_too_small(struct canvas *c) {
  static const char msg[] = "We're gonna need a bigger ocean";
  int len = (int)sizeof(msg) - 1;
  if (len > c->width) len = c->width;
  int x0 = (c->width - len) / 2;
  int y = c->height / 2;
  struct attr plain = {.col = COL_DEFAULT, .bold = false};
  for (int i = 0; i < len; i++) canvas_put(c, x0 + i, y, &msg[i], 1, plain, 1);
}

void app_init(struct app *a, int classic_ver, struct aquatic_life aquatic, double pace, int fps, int colors_mode, double now) {
  scene_init(&a->scene, classic_ver, aquatic);
  canvas_init(&a->canvas);
  a->w = -1;
  a->h = -1;
  a->scene_w = -1;
  a->scene_h = -1;
  a->paused = false;
  a->tick_accum = 0.0;
  a->pace = pace;
  a->fps = classic_ver != 0 ? CLASSIC_FPS : fps;
  a->colors_mode = colors_mode;
  a->last = now;
  settings_ui_init(&a->settings, &a->fps, &a->pace, &a->colors_mode, &a->scene);
  help_ui_init(&a->help, &a->scene);
}

void app_free(struct app *a) {
  canvas_free(&a->canvas);
  scene_free(&a->scene);
}

void app_resize(struct app *a, int w, int h) {
  if (w == a->w && h == a->h) return;
  canvas_resize(&a->canvas, w, h);
  a->w = w;
  a->h = h;
  if (too_small(a)) return;
  scene_resize(&a->scene, a->scene_w, a->scene_h, w, h);
  a->scene_w = w;
  a->scene_h = h;
}

void app_key(struct app *a, int key) {
  if (too_small(a)) return;
  if (key == 'r') scene_reset(&a->scene, a->w, a->h);
  if (key == 'p' || (key == ' ' && !settings_ui_is_open(&a->settings))) a->paused = !a->paused;
  if (key == 'f') app_feed(a, FEED_COL_AUTO);
  if (key == 's') {
    if (help_ui_is_open(&a->help)) help_ui_close(&a->help);
    settings_ui_toggle(&a->settings);
  }
  if (key == 'h') {
    if (settings_ui_is_open(&a->settings)) settings_ui_close(&a->settings);
    help_ui_toggle(&a->help);
  }
  if (settings_ui_is_open(&a->settings)) {
    if (key == '\x1b') settings_ui_close(&a->settings);
    else settings_ui_handle_key(&a->settings, key, a->w, a->h);
  }
  if (help_ui_is_open(&a->help) && key == '\x1b') help_ui_close(&a->help);
}

void app_feed(struct app *a, int col) {
  if (too_small(a)) return;
  scene_feed(&a->scene, a->w, a->h, col);
}

void app_click(struct app *a, int x, int y) {
  if (too_small(a)) return;
  if (settings_ui_click(&a->settings, x, y, a->w, a->h)) return;
  if (settings_ui_is_open(&a->settings)) {
    settings_ui_close(&a->settings);
    return;
  }
  if (help_ui_click(&a->help, x, y)) return;
  if (help_ui_is_open(&a->help)) {
    help_ui_close(&a->help);
    return;
  }
  app_feed(a, x);
}

void app_frame(struct app *a, double now) {
  double dt = now - a->last;
  a->last = now;
  if (dt < 0.0) dt = 0.0;
  if (dt > 0.5) dt = 0.5;
  if (!a->paused && !too_small(a)) {
    a->tick_accum += dt * 10.0 * a->pace;
    while (a->tick_accum >= 1.0) {
      scene_tick(&a->scene, a->w, a->h);
      a->tick_accum -= 1.0;
    }
  }
  canvas_clear(&a->canvas);
  if (too_small(a)) {
    draw_too_small(&a->canvas);
    return;
  }
  scene_draw(&a->scene, &a->canvas, a->tick_accum);
  canvas_reduce_colors(&a->canvas, a->colors_mode);
  help_ui_draw(&a->help, &a->canvas);
  settings_ui_draw(&a->settings, &a->canvas);
}
