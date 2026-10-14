package org.underthec.wear;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.KeyEvent;

import org.underthec.Options;
import org.underthec.SettingsActivity;
import org.underthec.UnderTheCView;
import org.underthec.lib.R;

public final class WearActivity extends Activity {
  private UnderTheCView view;

  @Override
  protected void onCreate(Bundle state) {
    super.onCreate(state);
    view = new UnderTheCView(this);
    view.setOnLongPressListener(this::showMenu);
    setContentView(view);
  }

  @Override
  protected void onResume() {
    super.onResume();
    SharedPreferences prefs = Options.prefs(this);
    view.configure(Options.build(this, prefs), Options.fontSize(prefs));
  }

  private void showMenu() {
    String[] items = {getString(R.string.menu_pause), getString(R.string.menu_settings)};
    new AlertDialog.Builder(this).setItems(items, (d, which) -> {
      if (which == 0) view.key('p');
      else startActivity(new Intent(this, SettingsActivity.class));
    }).show();
  }

  @Override
  public boolean onKeyDown(int keyCode, KeyEvent e) {
    if (keyCode == KeyEvent.KEYCODE_BACK && view.dialogOpen()) {
      view.key(0x1b);
      return true;
    }
    return super.onKeyDown(keyCode, e);
  }
}
