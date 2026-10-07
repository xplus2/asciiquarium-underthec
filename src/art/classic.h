#ifndef UNDERTHEC_ART_CLASSIC_H
#define UNDERTHEC_ART_CLASSIC_H

#include "../sprite.h"

static const char *const classic_castle_image[] = {
  "               T~~",
  "               |",
  "              /^\\",
  "             /   \\",
  " _   _   _  /     \\  _   _   _",
  "[ ]_[ ]_[ ]/ _   _ \\[ ]_[ ]_[ ]",
  "|_=__-_ =_|_[ ]_[ ]_|_=-___-__|",
  " | _- =  | =_ = _    |= _=   |",
  " |= -[]  |- = _ =    |_-=_[] |",
  " | =_    |= - ___    | =_ =  |",
  " |=  []- |-  /| |\\   |=_ =[] |",
  " |- =_   | =| | | |  |- = -  |",
  " |_______|__|_|_|_|__|_______|",
  NULL
};

static const char *const classic_castle_mask[] = {
  "                RR",
  "",
  "              yyy",
  "             y   y",
  "            y     y",
  "           y       y",
  "",
  "",
  "",
  "              yyy",
  "             yy yy",
  "            y y y y",
  "            yyyyyyy",
  NULL
};

static const struct sprite_pair classic_castle = { classic_castle_image, classic_castle_mask };

static const char *const classic_shark_image_0[] = {
  "                              __",
  "                             ( `\\",
  "  ,\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?)   `\\",
  ";' `.\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?(     `\\__",
  " ;   `.\?\?\?\?\?\?\?\?\?\?\?\?\?__..---''          `~~~~-._",
  "  `.   `.____...--''                       (b  `--._",
  "    >                     _.-'      .((      ._     )",
  "  .`.-`--...__         .-'     -.___.....-(|/|/|/|/'",
  " ;.'\?\?\?\?\?\?\?\?\?`. ...----`.___.',,,_______......---'",
  " '\?\?\?\?\?\?\?\?\?\?\?'-'",
  NULL
};

static const char *const classic_shark_mask_0[] = {
  "",
  "",
  "",
  "",
  "",
  "                                           cR",
  " ",
  "                                          cWWWWWWWW",
  NULL
};

static const char *const classic_shark_image_1[] = {
  "                     __",
  "                    /' )",
  "                  /'   (\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?,",
  "              __/'     )\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?.' `;",
  "      _.-~~~~'          ``---..__\?\?\?\?\?\?\?\?\?\?\?\?\?.'   ;",
  " _.--'  b)                       ``--...____.'   .'",
  "(     _.      )).      `-._                     <",
  " `\\|\\|\\|\\|)-.....___.-     `-.         __...--'-.'.",
  "   `---......_______,,,`.___.'----... .'\?\?\?\?\?\?\?\?\?`.;",
  "                                     `-`\?\?\?\?\?\?\?\?\?\?\?`",
  NULL
};

static const char *const classic_shark_mask_1[] = {
  "",
  "",
  "",
  "",
  "",
  "        Rc",
  "",
  "  WWWWWWWWc",
  NULL
};

static const struct sprite_pair classic_shark[2] = {
  { classic_shark_image_0, classic_shark_mask_0 },
  { classic_shark_image_1, classic_shark_mask_1 },
};

#define CLASSIC_SHARK_TEETH_ROW 7
#define CLASSIC_SHARK_TEETH_COL_RIGHT 44
#define CLASSIC_SHARK_TEETH_COL_LEFT 9

#endif
