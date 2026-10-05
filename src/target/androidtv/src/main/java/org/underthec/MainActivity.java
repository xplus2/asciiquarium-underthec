package org.underthec;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

public final class MainActivity extends Activity {
  private static final int KEY_UP = -2;
  private static final int KEY_DOWN = -3;
  private static final int KEY_LEFT = -4;
  private static final int KEY_RIGHT = -5;

  private UnderTheCView view;
  private boolean longPress;

  @Override
  protected void onCreate(Bundle state) {
    super.onCreate(state);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    view = new UnderTheCView(this);
    setContentView(view);
  }

  @Override
  protected void onResume() {
    super.onResume();
    SharedPreferences prefs = Options.prefs(this);
    view.configure(Options.build(prefs), Options.fontSize(prefs));
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus && Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      WindowInsetsController c = getWindow().getInsetsController();
      if (c != null) {
        c.hide(WindowInsets.Type.systemBars());
        c.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
      }
    }
  }

  private int map(int keyCode, KeyEvent e) {
    switch (keyCode) {
      case KeyEvent.KEYCODE_DPAD_UP:
        return KEY_UP;
      case KeyEvent.KEYCODE_DPAD_DOWN:
        return KEY_DOWN;
      case KeyEvent.KEYCODE_DPAD_LEFT:
        return KEY_LEFT;
      case KeyEvent.KEYCODE_DPAD_RIGHT:
        return KEY_RIGHT;
      case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
      case KeyEvent.KEYCODE_MEDIA_PLAY:
      case KeyEvent.KEYCODE_MEDIA_PAUSE:
        return 'p';
      case KeyEvent.KEYCODE_INFO:
        return 'h';
      case KeyEvent.KEYCODE_PROG_RED:
        return 'r';
      case KeyEvent.KEYCODE_MEDIA_FAST_FORWARD:
      case KeyEvent.KEYCODE_PAGE_UP:
      case KeyEvent.KEYCODE_CHANNEL_UP:
        return '+';
      case KeyEvent.KEYCODE_MEDIA_REWIND:
      case KeyEvent.KEYCODE_PAGE_DOWN:
      case KeyEvent.KEYCODE_CHANNEL_DOWN:
        return '-';
      default:
        int c = e.getUnicodeChar();
        return c > 0 && c < 128 ? c : 0;
    }
  }

  private void openSettings() {
    startActivity(new Intent(this, SettingsActivity.class));
  }

  @Override
  public boolean onKeyDown(int keyCode, KeyEvent e) {
    switch (keyCode) {
      case KeyEvent.KEYCODE_BACK:
      case KeyEvent.KEYCODE_DPAD_CENTER:
      case KeyEvent.KEYCODE_ENTER:
        if (e.getRepeatCount() == 0) {
          e.startTracking();
          longPress = false;
        }
        return true;
      case KeyEvent.KEYCODE_MENU:
      case KeyEvent.KEYCODE_SETTINGS:
      case KeyEvent.KEYCODE_DPAD_UP:
        if (e.getRepeatCount() == 0) openSettings();
        return true;
      default:
        break;
    }
    int key = map(keyCode, e);
    if (key == 0) return super.onKeyDown(keyCode, e);
    view.key(key);
    return true;
  }

  @Override
  public boolean onKeyLongPress(int keyCode, KeyEvent e) {
    switch (keyCode) {
      case KeyEvent.KEYCODE_BACK:
      case KeyEvent.KEYCODE_DPAD_CENTER:
      case KeyEvent.KEYCODE_ENTER:
        longPress = true;
        openSettings();
        return true;
      default:
        return super.onKeyLongPress(keyCode, e);
    }
  }

  @Override
  public boolean onKeyUp(int keyCode, KeyEvent e) {
    switch (keyCode) {
      case KeyEvent.KEYCODE_DPAD_CENTER:
      case KeyEvent.KEYCODE_ENTER:
        if (!longPress) view.key('f');
        return true;
      case KeyEvent.KEYCODE_BACK:
        if (longPress) return true;
        if (view.dialogOpen()) {
          view.key(0x1b);
          return true;
        }
        return super.onKeyUp(keyCode, e);
      default:
        return super.onKeyUp(keyCode, e);
    }
  }
}
