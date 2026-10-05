package org.underthec;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.os.Build;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Toast;

import java.util.Arrays;

public final class UnderTheCView extends SurfaceView implements SurfaceHolder.Callback, Choreographer.FrameCallback {
  private static final int MIN_FONT_PX = 6;
  private static final long SLACK_NANOS = 2_000_000L;

  private final Paint metrics = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final GlyphRenderer renderer = new GlyphRenderer();

  private long handle;
  private byte[][] opts = new byte[0][];
  private int fontPx;
  private int[] cells = new int[0];
  private int surfaceW;
  private int surfaceH;
  private boolean surfaceValid;
  private int cols;
  private int rows;
  private int cellW;
  private int cellH;
  private int offX;
  private int offY;
  private long lastNanos;
  private boolean running;

  public UnderTheCView(Context context) {
    super(context);
    metrics.setTypeface(Typeface.MONOSPACE);
    getHolder().addCallback(this);
    setFocusable(false);
  }

  public void configure(byte[][] newOpts, int newFontPx) {
    if (handle != 0 && newFontPx == fontPx && Arrays.deepEquals(newOpts, opts)) return;
    opts = newOpts;
    fontPx = newFontPx;
    if (isAttachedToWindow()) restart();
  }

  private void restart() {
    if (handle != 0) {
      Native.destroy(handle);
      handle = 0;
    }
    try {
      handle = Native.create(opts, System.nanoTime() / 1e9);
    } catch (IllegalArgumentException e) {
      Toast.makeText(getContext(), e.getMessage(), Toast.LENGTH_LONG).show();
      handle = Native.create(new byte[0][], System.nanoTime() / 1e9);
    }
    layoutGrid();
  }

  public void key(int key) {
    if (handle != 0) Native.key(handle, key);
  }

  public boolean dialogOpen() {
    return handle != 0 && Native.dialogOpen(handle);
  }

  private void layoutGrid() {
    if (handle == 0 || surfaceW <= 0 || surfaceH <= 0) return;
    float px = Math.max(MIN_FONT_PX, fontPx > 0 ? fontPx : surfaceH / 45f);
    metrics.setTextSize(px);
    cellW = Math.max(1, Math.round(metrics.measureText("M")));
    Paint.FontMetrics fm = metrics.getFontMetrics();
    cellH = Math.max(1, (int) Math.ceil(fm.descent - fm.ascent));
    renderer.reset(px, cellW, cellH, -fm.ascent);
    cols = Math.max(1, surfaceW / cellW);
    rows = Math.max(1, surfaceH / cellH);
    offX = (surfaceW - cols * cellW) / 2;
    offY = (surfaceH - rows * cellH) / 2;
    Native.resize(handle, cols, rows);
    cells = new int[cols * rows];
  }

  @Override
  public void surfaceCreated(SurfaceHolder holder) {
    surfaceValid = true;
  }

  @Override
  public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
    surfaceW = w;
    surfaceH = h;
    layoutGrid();
  }

  @Override
  public void surfaceDestroyed(SurfaceHolder holder) {
    surfaceValid = false;
  }

  @Override
  protected void onAttachedToWindow() {
    super.onAttachedToWindow();
    restart();
  }

  @Override
  protected void onDetachedFromWindow() {
    stopFrames();
    if (handle != 0) {
      Native.destroy(handle);
      handle = 0;
    }
    super.onDetachedFromWindow();
  }

  @Override
  protected void onWindowVisibilityChanged(int visibility) {
    super.onWindowVisibilityChanged(visibility);
    if (visibility == VISIBLE) startFrames();
    else stopFrames();
  }

  private void startFrames() {
    if (running) return;
    running = true;
    Choreographer.getInstance().postFrameCallback(this);
  }

  private void stopFrames() {
    running = false;
    Choreographer.getInstance().removeFrameCallback(this);
  }

  @Override
  public void doFrame(long frameTimeNanos) {
    if (!running) return;
    if (handle != 0 && surfaceValid) {
      long period = 1_000_000_000L / Math.max(1, Native.fps(handle));
      if (frameTimeNanos - lastNanos + SLACK_NANOS >= period) {
        lastNanos = frameTimeNanos;
        Native.frame(handle, frameTimeNanos / 1e9, cells);
        drawFrame();
      }
    }
    Choreographer.getInstance().postFrameCallback(this);
  }

  private void drawFrame() {
    SurfaceHolder holder = getHolder();
    boolean hardware = Build.VERSION.SDK_INT >= Build.VERSION_CODES.M;
    Canvas canvas = hardware ? holder.getSurface().lockHardwareCanvas() : holder.lockCanvas();
    if (canvas == null) return;
    try {
      canvas.drawColor(0xff000000);
      renderer.draw(canvas, cells, cols, rows, offX, offY);
    } finally {
      if (hardware) holder.getSurface().unlockCanvasAndPost(canvas);
      else holder.unlockCanvasAndPost(canvas);
    }
  }

  @Override
  public boolean onTouchEvent(MotionEvent e) {
    if (e.getAction() == MotionEvent.ACTION_DOWN) return true;
    if (e.getAction() != MotionEvent.ACTION_UP || handle == 0) return super.onTouchEvent(e);
    int col = (int) ((e.getX() - offX) / cellW);
    int row = (int) ((e.getY() - offY) / cellH);
    if (col >= 0 && col < cols && row >= 0 && row < rows) Native.click(handle, col, row);
    return true;
  }
}
