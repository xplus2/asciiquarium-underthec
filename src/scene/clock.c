#include "scene_internal.h"

#include <string.h>
#include <time.h>

#define HARDBLANK '$'
#define GLYPH_COUNT 15
#define SEQ_MAX 12

enum glyph { G_COLON = 10, G_SPACE = 11, G_A = 12, G_M = 13, G_P = 14 };

/* figlet small */
static const char *const glyphs[GLYPH_COUNT][CLOCK_ROWS] = {
  {"   __  ", "  /  \\ ", " | () |", "  \\__/ ", "       "},
  {"  _ ", " / |", " | |", " |_|", "    "},
  {"  ___ ", " |_  )", "  / / ", " /___|", "      "},
  {"  ____", " |__ /", "  |_ \\", " |___/", "      "},
  {"  _ _  ", " | | | ", " |_  _|", "   |_| ", "       "},
  {"  ___ ", " | __|", " |__ \\", " |___/", "      "},
  {"   __ ", "  / / ", " / _ \\", " \\___/", "      "},
  {"  ____ ", " |__  |", "   / / ", "  /_/  ", "       "},
  {"  ___ ", " ( _ )", " / _ \\", " \\___/", "      "},
  {"  ___ ", " / _ \\", " \\_, /", "  /_/ ", "      "},
  {"  _ ", " (_)", "  _ ", " (_)", "    "},
  {" $", " $", " $", " $", " $"},
  {"       ", "  __ _ ", " / _` |", " \\__,_|", "       "},
  {"        ", "  _ __  ", " | '  \\ ", " |_|_|_|", "        "},
  {"       ", "  _ __ ", " | '_ \\", " | .__/", " |_|   "},
};

static bool in_set(const char *set, char ch) {
  return ch != '\0' && strchr(set, ch) != NULL;
}

static char smush_pair(char l, char r) {
  static const char *const classes[] = {"|", "/\\", "[]", "{}", "()", "<>"};
  if (l == HARDBLANK || r == HARDBLANK) return '\0';
  if (l == r) return l;
  if (l == '_' && in_set("|/\\[]{}()<>", r)) return r;
  if (r == '_' && in_set("|/\\[]{}()<>", l)) return l;
  for (size_t i = 0; i < sizeof classes / sizeof *classes; i++) {
    for (size_t j = i + 1; j < sizeof classes / sizeof *classes; j++) {
      if (in_set(classes[i], l) && in_set(classes[j], r)) return r;
      if (in_set(classes[j], l) && in_set(classes[i], r)) return l;
    }
  }
  if (in_set("[]", l) && in_set("[]", r)) return '|';
  if (in_set("{}", l) && in_set("{}", r)) return '|';
  if (in_set("()", l) && in_set("()", r)) return '|';
  return '\0';
}

static char smush_cells(char l, char r, int lw, int rw) {
  if (l == ' ') return r;
  if (r == ' ') return l;
  if (lw < 2 || rw < 2) return '\0';
  return smush_pair(l, r);
}

static int smush_amount(const char buf[][CLOCK_COLS], int len, const char *const *g, int lw, int rw) {
  int best = rw;
  for (int row = 0; row < CLOCK_ROWS; row++) {
    int edge = len > 0 ? len - 1 : 0;
    while (edge > 0 && (buf[row][edge] == '\0' || buf[row][edge] == ' ')) edge--;
    int lead = 0;
    while (g[row][lead] == ' ') lead++;
    int amt = lead + len - 1 - edge;
    char l = buf[row][edge];
    if (l == '\0' || l == ' ') amt++;
    else if (g[row][lead] != '\0' && smush_cells(l, g[row][lead], lw, rw) != '\0') amt++;
    if (amt < best) best = amt;
  }
  return best;
}

static void render(char buf[][CLOCK_COLS], const int *seq, int count) {
  memset(buf, 0, (size_t)CLOCK_ROWS * CLOCK_COLS);
  int len = 0;
  int prev_w = 0;
  for (int i = 0; i < count; i++) {
    const char *const *g = glyphs[seq[i]];
    int w = (int)strlen(g[0]);
    int amt = smush_amount(buf, len, g, prev_w, w);
    for (int row = 0; row < CLOCK_ROWS; row++) {
      for (int k = 0; k < amt; k++) {
        int col = len - amt + k;
        if (col < 0) col = 0;
        buf[row][col] = smush_cells(buf[row][col], g[row][k], prev_w, w);
      }
      memcpy(buf[row] + strlen(buf[row]), g[row] + amt, (size_t)(w - amt) + 1);
    }
    len = (int)strlen(buf[0]);
    prev_w = w;
  }
  for (int row = 0; row < CLOCK_ROWS; row++) {
    for (char *p = buf[row]; *p != '\0'; p++) if (*p == HARDBLANK) *p = ' ';
  }
}

static int clock_sequence(int mode, const struct tm *tm, int *seq) {
  int n = 0;
  int hour = tm->tm_hour;
  if (mode == 12) {
    hour %= 12;
    if (hour == 0) hour = 12;
  }
  if (hour >= 10) seq[n++] = hour / 10;
  seq[n++] = hour % 10;
  seq[n++] = G_SPACE;
  seq[n++] = G_COLON;
  seq[n++] = G_SPACE;
  seq[n++] = tm->tm_min / 10;
  seq[n++] = tm->tm_min % 10;
  if (mode == 12) {
    seq[n++] = G_SPACE;
    seq[n++] = tm->tm_hour >= 12 ? G_P : G_A;
    seq[n++] = G_M;
  }
  return n;
}

static int ink_width(const struct scene *sc) {
  int w = 0;
  for (int row = 0; row < CLOCK_ROWS; row++) {
    int len = (int)strlen(sc->clock_buf[row]);
    while (len > 0 && sc->clock_buf[row][len - 1] == ' ') len--;
    if (len > w) w = len;
  }
  return w;
}

static bool clock_refresh(struct scene *sc) {
  time_t now = time(NULL);
  struct tm *tm = localtime(&now);
  if (tm == NULL) return false;
  int stamp = tm->tm_hour * 60 + tm->tm_min;
  if (stamp == sc->clock_stamp) return false;
  int seq[SEQ_MAX];
  render(sc->clock_buf, seq, clock_sequence(sc->time_mode, tm, seq));
  for (int row = 0; row < CLOCK_ROWS; row++) sc->clock_rows[row] = sc->clock_buf[row];
  sc->clock_rows[CLOCK_ROWS] = NULL;
  sc->clock_frame.shape = (ascii_rows)sc->clock_rows;
  sc->clock_frame.mask = NULL;
  sc->clock_stamp = stamp;
  return true;
}

void reposition_clock(const struct scene *sc, struct entity *e, int w, int h) {
  e->x = (w - ink_width(sc)) / 2;
  e->y = (h - CLOCK_ROWS) / 2;
  e->has_prev = false;
}

void add_clock(struct scene *sc, int w, int h) {
  sc->clock_stamp = -1;
  clock_refresh(sc);
  if (sc->clock_frame.shape == NULL) return;
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_CLOCK;
  e->z = Z_CLOCK;
  e->frames = &sc->clock_frame;
  e->frame_count = 1;
  e->sentinel = ' ';
  e->default_attr = sc->time_attr;
  reposition_clock(sc, e, w, h);
}

void clock_tick(struct scene *sc, int w, int h) {
  if (!clock_refresh(sc)) return;
  struct entity *e = entity_find_first(&sc->entities, ENT_CLOCK);
  if (e == NULL) return;
  entity_shape_changed(e);
  reposition_clock(sc, e, w, h);
}
