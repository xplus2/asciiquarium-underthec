#ifndef UNDERTHEC_ART_SAILBOAT_H
#define UNDERTHEC_ART_SAILBOAT_H

#include "../sprite.h"

static const char *const sailboat_image_0[] = {
  "  ~;",
  "   |\\",
  "  /| \\",
  " /_|__\\",
  "<======>",
  NULL
};

static const char *const sailboat_image_1[] = {
  "    ;~",
  "   /|",
  "  / |\\",
  " /__|_\\",
  "<======>",
  NULL
};

static const char *const sailboat_mask_0[] = {
  "  Yw",
  "   yb",
  "  by b",
  " bwywwb",
  "yyyyyyyy",
  NULL
};

static const char *const sailboat_mask_1[] = {
  "    wY",
  "   by",
  "  b yb",
  " bwwywb",
  "yyyyyyyy",
  NULL
};

static const struct sprite_pair sailboat[2] = {
  { sailboat_image_0, sailboat_mask_0 },
  { sailboat_image_1, sailboat_mask_1 },
};

#endif
