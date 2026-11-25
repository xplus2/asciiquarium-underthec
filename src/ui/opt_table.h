#ifndef UNDERTHEC_UI_OPT_TABLE_H
#define UNDERTHEC_UI_OPT_TABLE_H

/* screensaver dialog options, choice lists */
enum ot_opt {
  OT_CLASSIC, OT_AQUATIC, OT_MESSAGE, OT_COLOR, OT_POSITION, OT_CASTLE_NAME, OT_NO_CASTLE,
  OT_PACE, OT_UTURN, OT_FPS, OT_COLORS, OT_TIME, OT_TIME_COLOR, OT_AUTOFEED, OT_COUNT
};

#define OT_CLASSIC_N 3
#define OT_TIME_N 3
#define OT_COLORS_N 21
#define OT_COLOR_N 17
#define OT_POSITION_N 5

/* long CLI names */
extern const char *const ot_names[OT_COUNT];
/* dialog names in errors */
extern const char *const ot_labels[OT_COUNT];
extern const char *const ot_classic_items[OT_CLASSIC_N];
extern const char *const ot_time_items[OT_TIME_N];
extern const char *const ot_time_labels[OT_TIME_N];
extern const char *const ot_colors_items[OT_COLORS_N];
extern const char *const ot_colors_labels[OT_COLORS_N];
/* [0]: default */
extern const char *const ot_color_items[OT_COLOR_N];
extern const char *const ot_color_labels[OT_COLOR_N];
/* enum message_position order */
extern const char *const ot_position_items[OT_POSITION_N];

/* ot_colors_items index of colors mode, unknown: last */
int ot_colors_index(int mode);

#endif
