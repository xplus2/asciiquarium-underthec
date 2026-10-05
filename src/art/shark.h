#ifndef UNDERTHEC_ART_SHARK_H
#define UNDERTHEC_ART_SHARK_H

#include "../sprite.h"

static const char *const shark_image_0[] = {
  "                    _",
  "                   (`\\",
  "  ,\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?) `\\",
  ";' `.\?\?\?\?\?\?\?\?\?\?\?\?\?\?(   ``~~~..._",
  " `.  `.__...---''''          (b `--._",
  "   >            _.-'   .((    ._     )",
  " .`.-`-._     .'    -._...-(|/|/|/|/'",
  ";.'\?\?\?\?`. ..-`.__.',,,____.....---'",
  "'\?\?\?\?\?\?\?'-'",
  NULL
};

static const char *const shark_image_1[] = {
  "                 _",
  "                /')",
  "              /'  (\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?,",
  "      _...~~~''   )\?\?\?\?\?\?\?\?\?\?\?\?\?\?.' ';",
  " _.--' b)          ````---...__.'  .'",
  "(     _.    )).  `-._             <",
  " `\\|\\|\\|\\|)-..._.-   `-.     _.-'-.`.",
  "   `---.....____,,,`.__.'-.. .'\?\?\?\?'.;",
  "                            `-`\?\?\?\?\?\?'",
  NULL
};

static const char *const shark_mask_0[] = {
  "",
  "",
  "",
  "",
  "                             cR",
  " ",
  "                           cWWWWWWWW",
  NULL
};

static const char *const shark_mask_1[] = {
  "",
  "",
  "",
  "",
  "       Rc",
  "",
  "  WWWWWWWWc",
  NULL
};

static const struct sprite_pair shark[2] = {
  { shark_image_0, shark_mask_0 },
  { shark_image_1, shark_mask_1 },
};

#endif
