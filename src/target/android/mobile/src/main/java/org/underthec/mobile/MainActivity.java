package org.underthec.mobile;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

import org.underthec.Options;
import org.underthec.SettingsActivity;
import org.underthec.UnderTheCView;

public final class MainActivity extends Activity {
  private UnderTheCView view;

  @Override
  protected void onCreate(Bundle state) {
    super.onCreate(state);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      WindowManager.LayoutParams lp = getWindow().getAttributes();
      lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
      getWindow().setAttributes(lp);
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) getWindow().setDecorFitsSystemWindows(false);
    view = new UnderTheCView(this);
    view.setOnSwipeListener(() -> startActivity(new Intent(this, SettingsActivity.class)));
    setContentView(view);
  }

  @Override
  protected void onResume() {
    super.onResume();
    SharedPreferences prefs = Options.prefs(this);
    view.configure(Options.build(this, prefs), Options.fontSize(prefs));
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

  @Override
  public boolean onKeyDown(int keyCode, KeyEvent e) {
    if (keyCode == KeyEvent.KEYCODE_BACK && view.dialogOpen()) {
      view.key(0x1b);
      return true;
    }
    int c = e.getUnicodeChar();
    if (c > 0 && c < 128) {
      view.key(c);
      return true;
    }
    return super.onKeyDown(keyCode, e);
  }
}
