#ifndef UNDERTHEC_ART_KAIJU_H
#define UNDERTHEC_ART_KAIJU_H

#include "../sprite.h"

static const char *const kaiju_image_0[] = {
  "          _,-^^-._",
  "         /\\  ### /\\",
  "       _|{o\\  ^ /o}",
  "      _\\/  {~~''~~}",
  "     _\\/     wwww",
  "  _ _\\|     , NNN",
  " / }\\/       \\  \\\\",
  "{ {_/  }      ww|w",
  " \\    /   }\\    }",
  "  '~'^\\vwV}\?\\Vvvv}",
  NULL
};

static const char *const kaiju_mask_0[] = {
  "          22233222",
  "         22  222 22",
  "       322R2  2 2R2",
  "      332  22222222",
  "     332     wwww",
  "  2 332     2 www",
  " 2 232       2  22",
  "2 222  2      yy2y",
  " 2    2   22    2",
  "  22222yyy2 2yyyy2",
  NULL
};

static const char *const kaiju_image_1[] = {
  " _.-^^-,_",
  "/\\ ###  /\\",
  "{o\\ ^  /o}|_",
  "{~~''~~}  \\/_",
  "  wwww     \\/_",
  "  NNN ,     |/_ _",
  " //  /       \\/{ \\",
  " w|ww      {  \\_} }",
  "  {    /{   \\    /",
  " {vvvV/\?{Vwv/^'~'",
  NULL
};

static const char *const kaiju_mask_1[] = {
  " 22233222",
  "22 222  22",
  "2R2 2  2R223",
  "22222222  233",
  "  wwww     233",
  "  www 2     233 2",
  " 22  2       232 2",
  " y2yy      2  222 2",
  "  2    22   2    2",
  " 2yyyy2 2yyy22222",
  NULL
};

static const struct sprite_pair kaiju[2] = {
  { kaiju_image_0, kaiju_mask_0 },
  { kaiju_image_1, kaiju_mask_1 },
};

#endif
