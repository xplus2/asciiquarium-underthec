#ifndef UNDERTHEC_ART_ROWERS_H
#define UNDERTHEC_ART_ROWERS_H

#include "../sprite.h"

static const char *const rowers_image_0_0[] = {
  "  q\?\?q\?\?q\?\?q",
  "\\==\\==\\==\\==\\==/",
  "    \\\?\?\\\?\?\\\?\?\\",
  NULL
};

static const char *const rowers_image_0_1[] = {
  "   o\?\?o\?\?o\?\?o",
  "\\==|==|==|==|==/",
  "   |\?\?|\?\?|\?\?|",
  NULL
};

static const char *const rowers_image_0_2[] = {
  "    p\?\?p\?\?p\?\?p",
  "\\==/==/==/==/==/",
  "  /\?\?/\?\?/\?\?/",
  NULL
};

static const char *const rowers_image_0_3[] = {
  "___o__o__o__o",
  "\\==============/",
  NULL
};

static const char *const rowers_image_0_4[] = {
  "  q__q__q__q___",
  "\\==============/",
  NULL
};

static const char *const rowers_image_1_0[] = {
  "    p\?\?p\?\?p\?\?p  ",
  "\\==/==/==/==/==/",
  "  /\?\?/\?\?/\?\?/    ",
  NULL
};

static const char *const rowers_image_1_1[] = {
  "   o\?\?o\?\?o\?\?o   ",
  "\\==|==|==|==|==/",
  "   |\?\?|\?\?|\?\?|   ",
  NULL
};

static const char *const rowers_image_1_2[] = {
  "  q\?\?q\?\?q\?\?q    ",
  "\\==\\==\\==\\==\\==/",
  "    \\\?\?\\\?\?\\\?\?\\  ",
  NULL
};

static const char *const rowers_image_1_3[] = {
  "   o__o__o__o___",
  "\\==============/",
  NULL
};

static const char *const rowers_image_1_4[] = {
  " ___p__p__p__p  ",
  "\\==============/",
  NULL
};

static const char *const rowers_mask_0_0[] = {
  "  K  K  K  K",
  "111W11W11W11W111",
  "    W  W  W  W",
  NULL
};

static const char *const rowers_mask_0_1[] = {
  "   K  K  K  K",
  "111W11W11W11W111",
  "   W  W  W  W",
  NULL
};

static const char *const rowers_mask_0_2[] = {
  "    K  K  K  K",
  "111W11W11W11W111",
  "  W  W  W  W",
  NULL
};

static const char *const rowers_mask_0_3[] = {
  "WWWKWWKWWKWWK",
  "1111111111111111",
  "",
  NULL
};

static const char *const rowers_mask_0_4[] = {
  "  KWWKWWKWWKWWW",
  "1111111111111111",
  "",
  NULL
};

static const char *const rowers_mask_1_0[] = {
  "    K  K  K  K",
  "111W11W11W11W111",
  "  W  W  W  W",
  NULL
};

static const char *const rowers_mask_1_1[] = {
  "   K  K  K  K",
  "111W11W11W11W111",
  "   W  W  W  W",
  NULL
};

static const char *const rowers_mask_1_2[] = {
  "  K  K  K  K",
  "111W11W11W11W111",
  "    W  W  W  W",
  NULL
};

static const char *const rowers_mask_1_3[] = {
  "   KWWKWWKWWKWW",
  "1111111111111111",
  "",
  NULL
};

static const char *const rowers_mask_1_4[] = {
  " WWWKWWKWWKWWK",
  "1111111111111111",
  "",
  NULL
};

static const struct sprite_pair rowers[2][5] = {
  {
    {rowers_image_0_0, rowers_mask_0_0},
    {rowers_image_0_1, rowers_mask_0_1},
    {rowers_image_0_2, rowers_mask_0_2},
    {rowers_image_0_3, rowers_mask_0_3},
    {rowers_image_0_4, rowers_mask_0_4},
  },
  {
    {rowers_image_1_0, rowers_mask_1_0},
    {rowers_image_1_1, rowers_mask_1_1},
    {rowers_image_1_2, rowers_mask_1_2},
    {rowers_image_1_3, rowers_mask_1_3},
    {rowers_image_1_4, rowers_mask_1_4},
  },
};

static const char *const rowers_short_image_0_0[] = {
  "  q\?\?q    ",
  "\\==\\==\\==/",
  "    \\\?\?\\  ",
  NULL
};

static const char *const rowers_short_image_0_1[] = {
  "   o\?\?o   ",
  "\\==|==|==/",
  "   |\?\?|   ",
  NULL
};

static const char *const rowers_short_image_0_2[] = {
  "    p\?\?p  ",
  "\\==/==/==/",
  "  /\?\?/    ",
  NULL
};

static const char *const rowers_short_image_0_3[] = {
  "___o__o",
  "\\========/",
  NULL
};

static const char *const rowers_short_image_0_4[] = {
  "  q__q___",
  "\\========/",
  NULL
};

static const char *const rowers_short_image_1_0[] = {
  "    p\?\?p  ",
  "\\==/==/==/",
  "  /\?\?/    ",
  NULL
};

static const char *const rowers_short_image_1_1[] = {
  "   o\?\?o   ",
  "\\==|==|==/",
  "   |\?\?|   ",
  NULL
};

static const char *const rowers_short_image_1_2[] = {
  "  q\?\?q    ",
  "\\==\\==\\==/",
  "    \\\?\?\\  ",
  NULL
};

static const char *const rowers_short_image_1_3[] = {
  "   o__o___",
  "\\========/",
  NULL
};

static const char *const rowers_short_image_1_4[] = {
  " ___p__p  ",
  "\\========/",
  NULL
};

static const char *const rowers_short_mask_0_0[] = {
  "  K  K",
  "111W11W111",
  "    W  W",
  NULL
};

static const char *const rowers_short_mask_0_1[] = {
  "   K  K",
  "111W11W111",
  "   W  W",
  NULL
};

static const char *const rowers_short_mask_0_2[] = {
  "    K  K",
  "111W11W111",
  "  W  W",
  NULL
};

static const char *const rowers_short_mask_0_3[] = {
  "WWWKWWK",
  "1111111111",
  "",
  NULL
};

static const char *const rowers_short_mask_0_4[] = {
  "  KWWKWWW",
  "1111111111",
  "",
  NULL
};

static const char *const rowers_short_mask_1_0[] = {
  "    K  K",
  "111W11W111",
  "  W  W",
  NULL
};

static const char *const rowers_short_mask_1_1[] = {
  "   K  K",
  "111W11W111",
  "   W  W",
  NULL
};

static const char *const rowers_short_mask_1_2[] = {
  "  K  K",
  "111W11W111",
  "    W  W",
  NULL
};

static const char *const rowers_short_mask_1_3[] = {
  "   KWWKWWW",
  "1111111111",
  "",
  NULL
};

static const char *const rowers_short_mask_1_4[] = {
  " WWWKWWK",
  "1111111111",
  "",
  NULL
};

static const struct sprite_pair rowers_short[2][5] = {
  {
    {rowers_short_image_0_0, rowers_short_mask_0_0},
    {rowers_short_image_0_1, rowers_short_mask_0_1},
    {rowers_short_image_0_2, rowers_short_mask_0_2},
    {rowers_short_image_0_3, rowers_short_mask_0_3},
    {rowers_short_image_0_4, rowers_short_mask_0_4},
  },
  {
    {rowers_short_image_1_0, rowers_short_mask_1_0},
    {rowers_short_image_1_1, rowers_short_mask_1_1},
    {rowers_short_image_1_2, rowers_short_mask_1_2},
    {rowers_short_image_1_3, rowers_short_mask_1_3},
    {rowers_short_image_1_4, rowers_short_mask_1_4},
  },
};

#endif
