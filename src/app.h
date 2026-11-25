#ifndef UNDERTHEC_APP_H
#define UNDERTHEC_APP_H

#include <stdbool.h>

#include "canvas.h"
#include "ui/help.h"
#include "scene.h"
#include "ui/settings.h"

/* app_key arrows. web index.html + androidtv MainActivity mirror these */
#define APP_KEY_UP    (-2)
#define APP_KEY_DOWN  (-3)
#define APP_KEY_LEFT  (-4)
#define APP_KEY_RIGHT (-5)

#define APP_MIN_ROWS 15
#define APP_MIN_COLS 20

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
  int autofeed; /* mean secs, 0=off */
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

/* cells fitting w x h px, >= 1 each, centered offsets */
void app_grid_fit(int w, int h, int cell_w, int cell_h, int *cols, int *rows, int *off_x, int *off_y);

/* tick, canvas redraw */
void app_frame(struct app *a, double now);

/* advances *deadline by 1 frame, resyncs when >1 frame late. ret secs to wait, >= 0 */
double app_frame_wait(const struct app *a, double *deadline, double now);

#endif
