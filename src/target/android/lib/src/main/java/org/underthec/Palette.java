package org.underthec;

final class Palette {
  /* [color * 2 + bold] as 0xRRGGBB, from native color_rgb */
  private static final int[] RGB = Native.palette();

  private Palette() {
  }

  static int argb(int color, boolean bold) {
    return 0xff000000 | RGB[color * 2 + (bold ? 1 : 0)];
  }
}
