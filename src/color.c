#include "color.h"
#include "rng.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

static int color_checked = 0;
static bool color_ok = false;

bool color_supported(void) {
  if (color_checked) return color_ok;
  color_checked = 1;
  if (!isatty(STDOUT_FILENO)) {
    color_ok = false;
    return color_ok;
  }
  if (getenv("NO_COLOR") != NULL) {
    color_ok = false;
    return color_ok;
  }
  const char *term = getenv("TERM");
  if (term != NULL && strcmp(term, "dumb") == 0) {
    color_ok = false;
    return color_ok;
  }
  color_ok = true;
  return color_ok;
}

/* lowercase normal, uppercase bold. rest default */
static const struct attr mask_letter_attr[256] = {
  ['c'] = {.col = COL_CYAN},    ['C'] = {.col = COL_CYAN, .bold = true},
  ['r'] = {.col = COL_RED},     ['R'] = {.col = COL_RED, .bold = true},
  ['y'] = {.col = COL_YELLOW},  ['Y'] = {.col = COL_YELLOW, .bold = true},
  ['b'] = {.col = COL_BLUE},    ['B'] = {.col = COL_BLUE, .bold = true},
  ['g'] = {.col = COL_GREEN},   ['G'] = {.col = COL_GREEN, .bold = true},
  ['m'] = {.col = COL_MAGENTA}, ['M'] = {.col = COL_MAGENTA, .bold = true},
  ['w'] = {.col = COL_WHITE},   ['W'] = {.col = COL_WHITE, .bold = true},
  ['k'] = {.col = COL_BLACK},   ['K'] = {.col = COL_BLACK, .bold = true},
};

/* [enum color][bold] */
static const uint32_t palette_rgb[9][2] = {
  {0xc0c0c0, 0xffffff}, {0x000000, 0x808080}, {0xc00000, 0xff5555},
  {0x00c000, 0x55ff55}, {0xc0c000, 0xffff55}, {0x0000c0, 0x5555ff},
  {0xc000c0, 0xff55ff}, {0x00c0c0, 0x55ffff}, {0xc0c0c0, 0xffffff},
};

uint32_t color_rgb(enum color col, bool bold) {
  return palette_rgb[col][bold ? 1 : 0];
}

struct attr color_from_mask_letter(char c) {
  return mask_letter_attr[(unsigned char)c];
}

static const char *const color_names[8] = {"black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"};
static const enum color color_name_cols[8] = {COL_BLACK, COL_RED, COL_GREEN, COL_YELLOW, COL_BLUE, COL_MAGENTA, COL_CYAN, COL_WHITE};

/* -1: unknown */
static int color_name_index(const char *name) {
  if (name == NULL || name[0] == '\0') return -1;
  for (int i = 0; i < (int)(sizeof(color_names) / sizeof(color_names[0])); i++) {
    if (strcasecmp(name, color_names[i]) == 0) return i;
  }
  return -1;
}

struct attr color_from_name(const char *name) {
  int i = color_name_index(name);
  if (i < 0) return (struct attr){.col = COL_DEFAULT, .bold = false};
  return (struct attr){.col = color_name_cols[i], .bold = name[0] >= 'A' && name[0] <= 'Z'};
}

bool color_name_valid(const char *name) {
  return color_name_index(name) >= 0;
}

const char *const color_accent_names[COLOR_ACCENT_COUNT] = {"green", "red", "blue", "yellow", "magenta", "cyan", "white"};
static const enum color color_accent_cols[COLOR_ACCENT_COUNT] = {COL_GREEN, COL_RED, COL_BLUE, COL_YELLOW, COL_MAGENTA, COL_CYAN, COL_WHITE};

void color_mode_decode(int colors_mode, int *base, int *accent_id) {
  *base = colors_mode;
  *accent_id = 0;
  int ones = colors_mode % 10;
  if (colors_mode >= 11 && (ones == 1 || ones == 2)) {
    int id = colors_mode / 10;
    if (id >= 1 && id <= COLOR_ACCENT_COUNT) {
      *base = ones;
      *accent_id = id;
    }
  }
}

void color_reduce(int colors_mode, enum color *col, bool *bold) {
  int base;
  int accent_id;
  color_mode_decode(colors_mode, &base, &accent_id);
  switch (base) {
    case 1:
      *col = accent_id == 0 ? COL_WHITE : color_accent_cols[accent_id - 1];
      *bold = false;
      break;
    case 2:
      if (accent_id == 0) {
        if (*bold) *col = COL_WHITE;
        else *col = COL_BLACK;
      } else {
        *col = color_accent_cols[accent_id - 1];
      }
      *bold = !*bold;
      break;
    case 4:
      switch (*col) {
      case COL_RED: case COL_GREEN: case COL_BLUE: case COL_WHITE:
        break;
      case COL_YELLOW:
        *col = COL_WHITE;
        break;
      case COL_MAGENTA:
        *col = COL_RED;
        break;
      case COL_CYAN:
        *col = COL_BLUE;
        break;
      case COL_BLACK:
        if (*bold) *col = COL_WHITE;
        break;
      default:
        *col = COL_WHITE;
        break;
      }
      *bold = false;
      break;
    case 7:
      switch (*col) {
      case COL_RED: case COL_GREEN: case COL_YELLOW:
      case COL_BLUE: case COL_MAGENTA: case COL_CYAN:
        break;
      case COL_BLACK:
        if (*bold) *col = COL_WHITE;
        break;
      default:
        *col = COL_WHITE;
        break;
      }
      *bold = false;
      break;
    case 8:
      if (*col == COL_BLACK && *bold) *col = COL_WHITE;
      *bold = false;
      break;
    case 108:
      *bold = true;
      break;
    default:
      break;
  }
}

void color_roll_pick(char pick[10]) {
  static const char letters[] = {'c','C','r','R','y','Y','b','B','g','G','m','M'};
  pick[0] = '\0';
  for (int digit = 1; digit <= 9; digit++) pick[digit] = letters[rng_int((int)(sizeof(letters) / sizeof(letters[0])))];
}

void color_map_digits(const char *in, char *out, const char pick[10]) {
  size_t len = strlen(in);
  for (size_t i = 0; i < len; i++) {
    unsigned char c = (unsigned char)in[i];
    out[i] = c >= '0' && c <= '9' && pick[c - '0'] != '\0' ? pick[c - '0'] : in[i];
  }
  out[len] = '\0';
}
