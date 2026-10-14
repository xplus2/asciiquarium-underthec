package org.underthec;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.os.Build;
import android.os.PowerManager;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewConfiguration;
import android.widget.Toast;

import org.underthec.lib.R;

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
  private boolean powerSave;
  private final int powerSaveFps;
  private final float edgePx;
  private final float swipePx;
  private final int touchSlop;
  private float downX;
  private float downY;
  private Runnable onSwipe;
  private Runnable onLongPress;
  private boolean longPressed;
  private final Runnable longPressTask = () -> {
    longPressed = true;
    if (onLongPress != null) onLongPress.run();
  };

  private final BroadcastReceiver powerReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(Context c, Intent i) {
      updatePowerSave();
    }
  };

  public UnderTheCView(Context context) {
    super(context);
    metrics.setTypeface(Typeface.MONOSPACE);
    getHolder().addCallback(this);
    setFocusable(false);
    setSoundEffectsEnabled(false);
    float density = getResources().getDisplayMetrics().density;
    powerSaveFps = getResources().getInteger(R.integer.power_save_fps);
    edgePx = 32 * density;
    touchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
    swipePx = Math.max(touchSlop * 3f, 48 * density);
  }

  public void setOnSwipeListener(Runnable r) {
    onSwipe = r;
  }

  public void setOnLongPressListener(Runnable r) {
    onLongPress = r;
  }

  private void updatePowerSave() {
    PowerManager pm = (PowerManager) getContext().getSystemService(Context.POWER_SERVICE);
    powerSave = powerSaveFps > 0 && pm != null && pm.isPowerSaveMode();
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
    float px = Math.max(MIN_FONT_PX, fontPx > 0 ? fontPx : Math.min(surfaceW, surfaceH) / 45f);
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
    if (powerSaveFps > 0) {
      getContext().registerReceiver(powerReceiver, new IntentFilter(PowerManager.ACTION_POWER_SAVE_MODE_CHANGED));
      updatePowerSave();
    }
    restart();
  }

  @Override
  protected void onDetachedFromWindow() {
    stopFrames();
    if (powerSaveFps > 0) getContext().unregisterReceiver(powerReceiver);
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
      int fps = Native.fps(handle);
      if (powerSave) fps = Math.min(fps, powerSaveFps);
      long period = 1_000_000_000L / Math.max(1, fps);
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
    if (handle == 0) return super.onTouchEvent(e);
    switch (e.getAction()) {
      case MotionEvent.ACTION_DOWN:
        downX = e.getX();
        downY = e.getY();
        longPressed = false;
        if (onLongPress != null) postDelayed(longPressTask, ViewConfiguration.getLongPressTimeout());
        return true;
      case MotionEvent.ACTION_MOVE:
        if (Math.hypot(e.getX() - downX, e.getY() - downY) > touchSlop) removeCallbacks(longPressTask);
        return true;
      case MotionEvent.ACTION_CANCEL:
        removeCallbacks(longPressTask);
        return true;
      case MotionEvent.ACTION_UP:
        removeCallbacks(longPressTask);
        if (longPressed) return true;
        float dx = e.getX() - downX;
        float dy = e.getY() - downY;
        if (Math.max(Math.abs(dx), Math.abs(dy)) >= swipePx) {
          if (onSwipe != null && swipeAllowed(dx, dy)) onSwipe.run();
          return true;
        }
        int col = (int) ((e.getX() - offX) / cellW);
        int row = (int) ((e.getY() - offY) / cellH);
        if (col >= 0 && col < cols && row >= 0 && row < rows) Native.click(handle, col, row);
        performClick();
        return true;
      default:
        return true;
    }
  }

  @Override
  public boolean performClick() {
    return super.performClick();
  }

  private boolean swipeAllowed(float dx, float dy) {
    boolean horizontal = Math.abs(dx) > Math.abs(dy);
    if (!horizontal && dy > 0) return false;
    if (downX < edgePx || downX > getWidth() - edgePx || downY > getHeight() - edgePx) return false;
    return true;
  }
}
