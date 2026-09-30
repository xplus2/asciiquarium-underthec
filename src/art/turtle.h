#ifndef UNDERTHEC_ART_TURTLE_H
#define UNDERTHEC_ART_TURTLE_H

#include "../sprite.h"

static const char *const turtle_image_0[] = {
  "    ___   _",
  "   /# #\\ /.)",
  " ~(#_#_#Y;'",
  "  ()---()'",
  NULL
};

static const char *const turtle_mask_0[] = {
  "    222   G",
  "   22 22 GwG",
  " G222222GGG",
  "  GGyyyGGy",
  NULL
};

static const char *const turtle_image_1[] = {
  " _\?\?\?___",
  "(.\\\?/# #\\",
  " ';Y#_#_#)~",
  "  '()---()",
  NULL
};

static const char *const turtle_mask_1[] = {
  " G   222",
  "GwG 22 22",
  " GGG222222G",
  "  yGGyyyGG",
  NULL
};

/* 2: dark green, dark red or bold red shell, never blue. nobody likes the blue shell. */
static const struct sprite_pair turtle[2] = {
{ turtle_image_0, turtle_mask_0 },
{ turtle_image_1, turtle_mask_1 },
};

#endif
