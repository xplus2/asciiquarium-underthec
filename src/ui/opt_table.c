#include "opt_table.h"
#include "../opts.h"

const char *const ot_names[OT_COUNT] = {
  "classic", "aquatic-life", "message", "message-color", "message-position", "castle-name", "no-castle",
  "pace", "uturn-chance", "fps", "colors", "time", "time-color", "autofeed",
};

const char *const ot_labels[OT_COUNT] = {
  "Classic", "Fish", "Message", "Color", "Position", "Castle name", "Castle",
  "Pace", "U-turn chance", "FPS", "Colors", "Time", "Time color", "Autofeed",
};

const char *const ot_classic_items[OT_CLASSIC_N] = {"off", "1.0", "1.1"};
const char *const ot_time_items[OT_TIME_N] = {"off", "24", "12"};
const char *const ot_time_labels[OT_TIME_N] = {"Off", "24 hours", "12 hours (am/pm)"};

const char *const ot_colors_items[OT_COLORS_N] = {
  "1", "1-green", "1-red", "1-blue", "1-yellow", "1-magenta", "1-cyan", "1-white",
  "2", "2-green", "2-red", "2-blue", "2-yellow", "2-magenta", "2-cyan", "2-white",
  "4", "7", "8", "8-bold", "16",
};

const char *const ot_colors_labels[OT_COLORS_N] = {
  "1", "1-green", "1-red", "1-blue", "1-yellow", "1-magenta", "1-cyan", "1-white",
  "2", "2-green", "2-red", "2-blue", "2-yellow", "2-magenta", "2-cyan", "2-white",
  "4 (RGBW)", "7 (Teletext)", "8", "8-bold", "16 (ANSI)",
};

const char *const ot_color_items[OT_COLOR_N] = {
  "(default)", "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white",
  "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White",
};

const char *const ot_color_labels[OT_COLOR_N] = {
  "(default)", "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white",
  "Black (bold)", "Red (bold)", "Green (bold)", "Yellow (bold)", "Blue (bold)", "Magenta (bold)", "Cyan (bold)", "White (bold)",
};

const char *const ot_position_items[OT_POSITION_N] = {"middle", "center", "marquee", "swim", "event"};

int ot_colors_index(int mode) {
  char eb[128];
  for (int i = 0; i < OT_COLORS_N; i++) {
    int parsed;
    if (opts_parse_colors(ot_colors_items[i], &parsed, eb, sizeof eb) && parsed == mode) return i;
  }
  return OT_COLORS_N - 1;
}
