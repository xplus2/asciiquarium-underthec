package org.underthec;

final class Native {
  static {
    System.loadLibrary("underthec_android");
  }

  private Native() {
  }

  static native long create(byte[][] opts, double now, long wallMs);

  static native void destroy(long h);

  static native void resize(long h, int cols, int rows);

  static native void key(long h, int key);

  static native void click(long h, int col, int row);

  /* false: canvas unchanged, out untouched. force: full repack */
  static native boolean frame(long h, double now, long wallMs, int[] out, boolean force);

  static native int width(long h);

  static native int height(long h);

  static native int fps(long h);

  static native boolean dialogOpen(long h);

  /* 0xRRGGBB, [color * 2 + bold] */
  static native int[] palette();

  /* aquatic-life names, settings order */
  static native String[] creatures();

  /* option name/value pairs: core defaults */
  static native String[] defaults();
}
