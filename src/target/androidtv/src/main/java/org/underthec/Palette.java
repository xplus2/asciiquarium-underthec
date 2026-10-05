package org.underthec;

final class Palette {
  /* [enum color][bold] as xscr_font palette_rgb */
  private static final int[][] ARGB = {
      {0xffc0c0c0, 0xffffffff}, {0xff000000, 0xff808080}, {0xffc00000, 0xffff5555},
      {0xff00c000, 0xff55ff55}, {0xffc0c000, 0xffffff55}, {0xff0000c0, 0xff5555ff},
      {0xffc000c0, 0xffff55ff}, {0xff00c0c0, 0xff55ffff}, {0xffc0c0c0, 0xffffffff},
  };

  private Palette() {
  }

  static int argb(int color, boolean bold) {
    return ARGB[color][bold ? 1 : 0];
  }
}
