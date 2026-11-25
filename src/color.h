#ifndef UNDERTHEC_COLOR_H
#define UNDERTHEC_COLOR_H

#include <stdbool.h>
#include <stdint.h>

enum color {
  COL_DEFAULT = 0,
  COL_BLACK,
  COL_RED,
  COL_GREEN,
  COL_YELLOW,
  COL_BLUE,
  COL_MAGENTA,
  COL_CYAN,
  COL_WHITE
};

struct attr {
  enum color col;
  bool bold;
  enum color bg;
  bool bg_bold;
};

/* detect tty color support */
bool color_supported(void);

/* mask letters */
struct attr color_from_mask_letter(char c);
struct attr color_from_name(const char *name);
bool color_name_valid(const char *name);
/* pick[1..9]: random mask letter, pick[0] unset */
void color_roll_pick(char pick[10]);
/* digit d becomes pick[d] when set ('\0' = keep) */
void color_map_digits(const char *in, char *out, const char pick[10]);
void color_reduce(int colors_mode, enum color *col, bool *bold);
/* display palette, 0xRRGGBB */
uint32_t color_rgb(enum color col, bool bold);

#define COLOR_ACCENT_COUNT 7
extern const char *const color_accent_names[COLOR_ACCENT_COUNT];
void color_mode_decode(int colors_mode, int *base, int *accent_id);

#endif
