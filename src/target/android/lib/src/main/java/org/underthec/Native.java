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

  static native void frame(long h, double now, long wallMs, int[] out);

  static native int width(long h);

  static native int height(long h);

  static native int fps(long h);

  static native boolean dialogOpen(long h);
}
