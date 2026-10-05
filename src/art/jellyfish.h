#ifndef UNDERTHEC_ART_JELLYFISH_H
#define UNDERTHEC_ART_JELLYFISH_H

#include "../sprite.h"

static const char *const jellyfish_frame_0[] = {
  "  ___  ",
  " /   \\ ",
  "(_____)",
  " :\?;\?: ",
  " ;\?:\?; ",
  " :\?;\?: ",
  NULL
};

static const char *const jellyfish_frame_1[] = {
  "  ___  ",
  " /   \\ ",
  "(_____)",
  " ;\?:\?; ",
  " :\?;\?: ",
  " ;\?:\?; ",
  NULL
};

static const char *const jellyfish_mask_0[] = {
  "  111  ",
  " 1   1 ",
  "1111111",
  " m M m ",
  " M m M ",
  " m M m ",
  NULL
};

static const char *const jellyfish_mask_1[] = {
  "  111  ",
  " 1   1 ",
  "1111111",
  " M m M ",
  " m M m ",
  " M m M ",
  NULL
};

static const char *const jellyfish_small_frame_0[] = {
  " .-.",
  "(___)",
  " :;:",
  " ;:;",
  NULL
};

static const char *const jellyfish_small_frame_1[] = {
  " .-.",
  "(___)",
  " ;:;",
  " :;:",
  NULL
};

static const char *const jellyfish_small_mask_0[] = {
  " 111",
  "11111",
  " mMm",
  " MmM",
  NULL
};

static const char *const jellyfish_small_mask_1[] = {
  " 111",
  "11111",
  " MmM",
  " mMm",
  NULL
};

static const struct sprite_pair jellyfish_frames[2] = {
  { jellyfish_frame_0, jellyfish_mask_0 },
  { jellyfish_frame_1, jellyfish_mask_1 },
};

static const struct sprite_pair jellyfish_small_frames[2] = {
  { jellyfish_small_frame_0, jellyfish_small_mask_0 },
  { jellyfish_small_frame_1, jellyfish_small_mask_1 },
};

#endif
