package org.underthec;

import android.content.SharedPreferences;
import android.service.dreams.DreamService;

public final class UnderTheCDream extends DreamService {
  @Override
  public void onAttachedToWindow() {
    super.onAttachedToWindow();
    setInteractive(false);
    setFullscreen(true);
    UnderTheCView view = new UnderTheCView(this);
    SharedPreferences prefs = Options.prefs(this);
    view.configure(Options.build(prefs), Options.fontSize(prefs));
    setContentView(view);
  }
}
