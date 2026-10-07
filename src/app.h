#ifndef UNDERTHEC_APP_H
#define UNDERTHEC_APP_H

#include <stdbool.h>

#include "canvas.h"
#include "ui/help.h"
#include "scene.h"
#include "ui/settings.h"

/* dialogs point into struct, no copies after init */
#define APP_MIN_ROWS 15

struct app {
  struct scene scene;
  struct canvas canvas;
  struct settings_ui settings;
  struct help_ui help;
  int w;
  int h;
  int scene_w;
  int scene_h;
  bool paused;
  double tick_accum;
  double pace;
  int fps;
  int colors_mode;
  double last;
};

/* now=secs */
/* classic_ver != 0: fps fixed, lineup fixed */
void app_init(struct app *a, int classic_ver, struct aquatic_life aquatic, double pace, int fps, int colors_mode, double now);
void app_free(struct app *a);

void app_resize(struct app *a, int w, int h);

/* r,p,f,s,h,esc + dialog keys */
void app_key(struct app *a, int key);
/* column: click/tap pos or FEED_COL_AUTO */
void app_feed(struct app *a, int col);
/* dialog hits, otherwise: feed */
void app_click(struct app *a, int x, int y);

/* tick, canvas redraw */
void app_frame(struct app *a, double now);

#endif
