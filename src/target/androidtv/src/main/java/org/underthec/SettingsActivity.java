package org.underthec;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Bundle;
import android.text.InputType;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public final class SettingsActivity extends Activity {
  private static final int SWITCH = 0;
  private static final int STEP = 1;
  private static final int CHOICE = 2;
  private static final int TEXT = 3;
  private static final int SUBMENU = 4;

  private static final String KEY_FISH_AUTO = "fish-auto";
  private static final String KEY_FONT_SIZE = "font-size";
  private static final String KEY_CLASSIC = "classic";
  private static final String KEY_NO_CASTLE = "no-castle";
  private static final String KEY_CREATURE = "creature.";
  private static final String KEY_CASTLE = "castle";
  private static final String KEY_AQUATIC_LIFE = "aquatic-life";

  private static final int ALWAYS = 0;
  private static final int FREE_LIFE = 1;
  private static final int FISH_COUNT = 2;
  private static final int CASTLE_ON = 3;

  private static final class Row {
    int type;
    String key;
    String title;
    int rule = ALWAYS;
    int min;
    int max;
    int step = 1;
    int scale = 1;
    int def;
    String[] values;
    String[] labels;
    String defText = "";
    boolean multiline;
    List<Row> children;
  }

  private SharedPreferences prefs;
  private final List<Row> rootRows = new ArrayList<>();
  private List<Row> rows = rootRows;
  private Row openMenu;
  private ListView list;
  private TextView title;
  private TextView status;
  private BaseAdapter adapter;
  private int lastFish = 20;

  @Override
  protected void onCreate(Bundle state) {
    super.onCreate(state);
    prefs = Options.prefs(this);
    buildRows();

    LinearLayout root = new LinearLayout(this);
    root.setOrientation(LinearLayout.VERTICAL);
    root.setPadding(dp(48), dp(24), dp(48), dp(24));

    title = new TextView(this);
    title.setText(R.string.settings_title);
    title.setTextSize(24);
    root.addView(title);

    status = new TextView(this);
    status.setTextColor(Color.rgb(255, 85, 85));
    root.addView(status);

    list = new ListView(this);
    adapter = new RowAdapter();
    list.setAdapter(adapter);
    list.setItemsCanFocus(false);
    list.setOnItemClickListener((parent, view, position, id) -> activate(rows.get(position)));
    root.addView(list, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f));
    setContentView(root);
    validate();
  }

  private int dp(int v) {
    return Math.round(v * getResources().getDisplayMetrics().density);
  }

  private Row sw(String key, String title, int rule) {
    Row r = new Row();
    r.type = SWITCH;
    r.key = key;
    r.title = title;
    r.rule = rule;
    return r;
  }

  private Row step(String key, String title, int min, int max, int step, int def, int scale, int rule) {
    Row r = new Row();
    r.type = STEP;
    r.key = key;
    r.title = title;
    r.min = min;
    r.max = max;
    r.step = step;
    r.def = def;
    r.scale = scale;
    r.rule = rule;
    return r;
  }

  private Row choice(String key, String title, String[] values, String[] labels, String def) {
    Row r = new Row();
    r.type = CHOICE;
    r.key = key;
    r.title = title;
    r.values = values;
    r.labels = labels;
    r.defText = def;
    return r;
  }

  private Row text(String key, String title, boolean multiline, int rule) {
    Row r = new Row();
    r.type = TEXT;
    r.key = key;
    r.title = title;
    r.multiline = multiline;
    r.rule = rule;
    return r;
  }

  private Row submenu(String title, List<Row> children) {
    Row r = new Row();
    r.type = SUBMENU;
    r.title = title;
    r.children = children;
    return r;
  }

  private void buildRows() {
    List<Row> life = new ArrayList<>();
    life.add(sw(KEY_FISH_AUTO, getString(R.string.fish_auto), FREE_LIFE));
    life.add(step("fish", getString(R.string.fish_count), 0, 999, 1, 20, 1, FISH_COUNT));
    for (String c : Options.CREATURES) {
      life.add(sw(KEY_CREATURE + c, getString(R.string.creature, c), FREE_LIFE));
    }
    life.add(sw(KEY_CASTLE, getString(R.string.castle), ALWAYS));
    life.add(text("castle-name", getString(R.string.castle_name), false, CASTLE_ON));

    List<Row> msg = new ArrayList<>();
    msg.add(text("message", getString(R.string.message), true, ALWAYS));
    String[] colorLabels = Options.MESSAGE_COLORS.clone();
    colorLabels[0] = getString(R.string.default_value);
    msg.add(choice("message-color", getString(R.string.message_color), Options.MESSAGE_COLORS, colorLabels, ""));
    msg.add(choice("message-position", getString(R.string.message_position), Options.POSITIONS,
        Options.POSITIONS, "middle"));

    rootRows.add(choice("colors", getString(R.string.colors), Options.COLORS, Options.COLORS_LABELS, "16"));
    String[] sizes = new String[24];
    String[] sizeLabels = new String[24];
    sizes[0] = "0";
    sizeLabels[0] = getString(R.string.automatic);
    for (int i = 1; i < sizes.length; i++) {
      sizes[i] = String.valueOf(4 + 2 * i);
      sizeLabels[i] = sizes[i];
    }
    rootRows.add(choice(KEY_FONT_SIZE, getString(R.string.font_size), sizes, sizeLabels, "0"));
    rootRows.add(submenu(getString(R.string.aquatic_life), life));
    rootRows.add(submenu(getString(R.string.message_menu), msg));
    rootRows.add(step("pace", getString(R.string.pace), 1, 1000, 5, 100, 100, ALWAYS));
    rootRows.add(step("uturn-chance", getString(R.string.uturn_chance), 0, 999, 10, 400, 1, ALWAYS));
    rootRows.add(step("fps", getString(R.string.fps), 1, 240, 1, 24, 1, ALWAYS));
    rootRows.add(choice(KEY_CLASSIC, getString(R.string.classic), new String[] {"", "1.0", "1.1"},
        new String[] {getString(R.string.classic_off), "1.0", "1.1"}, ""));
  }

  private Options.Life life() {
    return Options.Life.parse(prefs.getString(KEY_AQUATIC_LIFE, Options.DEFAULT_LIFE));
  }

  private boolean enabled(Row r) {
    switch (r.rule) {
      case FREE_LIFE:
        return prefs.getString(KEY_CLASSIC, "").isEmpty();
      case FISH_COUNT:
        return prefs.getString(KEY_CLASSIC, "").isEmpty() && !life().auto;
      case CASTLE_ON:
        return prefs.getString(KEY_NO_CASTLE, "0").equals("0");
      default:
        return true;
    }
  }

  private boolean getBool(Row r) {
    if (r.key.equals(KEY_FISH_AUTO)) return life().auto;
    if (r.key.equals(KEY_CASTLE)) return prefs.getString(KEY_NO_CASTLE, "0").equals("0");
    return life().creatures.contains(r.key.substring(KEY_CREATURE.length()));
  }

  private void setBool(Row r, boolean v) {
    if (r.key.equals(KEY_CASTLE)) {
      prefs.edit().putString(KEY_NO_CASTLE, v ? "0" : "1").apply();
      return;
    }
    Options.Life l = life();
    if (r.key.equals(KEY_FISH_AUTO)) {
      l.auto = v;
      if (!v) l.fish = lastFish;
    } else {
      String name = r.key.substring(KEY_CREATURE.length());
      if (v) l.creatures.add(name);
      else l.creatures.remove(name);
    }
    prefs.edit().putString(KEY_AQUATIC_LIFE, l.toString()).apply();
  }

  private int getInt(Row r) {
    if (r.key.equals("fish")) return life().fish;
    String v = prefs.getString(r.key, null);
    if (v == null) return r.def;
    try {
      return (int) Math.round(Double.parseDouble(v) * r.scale);
    } catch (NumberFormatException e) {
      return r.def;
    }
  }

  private void setInt(Row r, int v) {
    int n = Math.max(r.min, Math.min(r.max, v));
    if (r.key.equals("fish")) {
      Options.Life l = life();
      l.auto = false;
      l.fish = n;
      lastFish = n;
      prefs.edit().putString(KEY_AQUATIC_LIFE, l.toString()).apply();
    } else if (r.scale == 100) {
      prefs.edit().putString(r.key, String.format(Locale.US, "%.2f", n / 100.0)).apply();
    } else {
      prefs.edit().putString(r.key, String.valueOf(n)).apply();
    }
  }

  private int choiceIndex(Row r) {
    if (r.key.equals(KEY_FONT_SIZE)) {
      int cur = prefs.getInt(KEY_FONT_SIZE, 0);
      int best = 0;
      for (int i = 1; i < r.values.length; i++) {
        if (Math.abs(Integer.parseInt(r.values[i]) - cur) < Math.abs(Integer.parseInt(r.values[best]) - cur)) best = i;
      }
      return cur == 0 ? 0 : best;
    }
    String cur = prefs.getString(r.key, r.defText);
    for (int i = 0; i < r.values.length; i++) {
      if (r.values[i].equals(cur)) return i;
    }
    return 0;
  }

  private void setChoice(Row r, int idx) {
    if (r.key.equals(KEY_FONT_SIZE)) prefs.edit().putInt(KEY_FONT_SIZE, Integer.parseInt(r.values[idx])).apply();
    else prefs.edit().putString(r.key, r.values[idx]).apply();
    changed();
  }

  private String stepText(Row r, int v) {
    if (r.scale == 100) return String.format(Locale.US, "%.2f", v / 100.0);
    return String.valueOf(v);
  }

  private void changed() {
    adapter.notifyDataSetChanged();
    validate();
  }

  private void validate() {
    String err = Options.validate(prefs);
    status.setText(err == null ? "" : err);
  }

  private boolean adjust(Row r, int dir) {
    if (!enabled(r)) return false;
    switch (r.type) {
      case SWITCH:
        setBool(r, !getBool(r));
        changed();
        return true;
      case STEP:
        setInt(r, getInt(r) + dir * r.step);
        changed();
        return true;
      case CHOICE:
        int n = r.values.length;
        setChoice(r, (choiceIndex(r) + dir + n) % n);
        return true;
      default:
        return false;
    }
  }

  private void activate(Row r) {
    if (!enabled(r)) return;
    switch (r.type) {
      case SWITCH:
        adjust(r, 1);
        break;
      case SUBMENU:
        show(r);
        break;
      case STEP:
        askNumber(r);
        break;
      case CHOICE:
        new AlertDialog.Builder(this)
            .setTitle(r.title)
            .setSingleChoiceItems(r.labels, choiceIndex(r), (d, which) -> {
              setChoice(r, which);
              d.dismiss();
            })
            .show();
        break;
      default:
        askText(r);
        break;
    }
  }

  private void show(Row menu) {
    openMenu = menu;
    rows = menu == null ? rootRows : menu.children;
    title.setText(menu == null ? getString(R.string.settings_title) : menu.title);
    adapter.notifyDataSetChanged();
    list.setSelection(0);
  }

  private void askNumber(Row r) {
    EditText in = new EditText(this);
    in.setInputType(r.scale == 100 ? InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL
        : InputType.TYPE_CLASS_NUMBER);
    in.setText(r.scale == 100 ? String.format(Locale.US, "%.2f", getInt(r) / 100.0) : String.valueOf(getInt(r)));
    in.setSelectAllOnFocus(true);
    new AlertDialog.Builder(this)
        .setTitle(r.title)
        .setView(in)
        .setPositiveButton(R.string.ok, (d, which) -> {
          try {
            double parsed = Double.parseDouble(in.getText().toString().trim());
            setInt(r, (int) Math.round(parsed * r.scale));
            changed();
          } catch (NumberFormatException ignored) {
            validate();
          }
        })
        .setNegativeButton(R.string.cancel, null)
        .show();
  }

  private void askText(Row r) {
    EditText in = new EditText(this);
    in.setGravity(Gravity.TOP | Gravity.START);
    if (r.multiline) {
      in.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_MULTI_LINE);
      in.setMinLines(3);
    } else {
      in.setInputType(InputType.TYPE_CLASS_TEXT);
      in.setSingleLine(true);
    }
    in.setText(prefs.getString(r.key, r.defText));
    new AlertDialog.Builder(this)
        .setTitle(r.title)
        .setView(in)
        .setPositiveButton(R.string.ok, (d, which) -> {
          prefs.edit().putString(r.key, in.getText().toString()).apply();
          changed();
        })
        .setNegativeButton(R.string.cancel, null)
        .show();
  }

  @Override
  public boolean onKeyDown(int keyCode, KeyEvent e) {
    if (keyCode == KeyEvent.KEYCODE_BACK && openMenu != null) {
      Row menu = openMenu;
      show(null);
      list.setSelection(rootRows.indexOf(menu));
      return true;
    }
    if (keyCode == KeyEvent.KEYCODE_DPAD_LEFT || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) {
      int pos = list.getSelectedItemPosition();
      int dir = keyCode == KeyEvent.KEYCODE_DPAD_LEFT ? -1 : 1;
      if (pos >= 0 && pos < rows.size() && adjust(rows.get(pos), dir)) return true;
    }
    return super.onKeyDown(keyCode, e);
  }

  private final class RowAdapter extends BaseAdapter {
    @Override
    public int getCount() {
      return rows.size();
    }

    @Override
    public Object getItem(int position) {
      return rows.get(position);
    }

    @Override
    public long getItemId(int position) {
      return position;
    }

    @Override
    public boolean areAllItemsEnabled() {
      return false;
    }

    @Override
    public boolean isEnabled(int position) {
      return enabled(rows.get(position));
    }

    private String valueText(Row r) {
      switch (r.type) {
        case SWITCH:
          return getString(getBool(r) ? R.string.on : R.string.off);
        case STEP:
          return stepText(r, getInt(r));
        case CHOICE:
          return r.labels[choiceIndex(r)];
        case SUBMENU:
          return ">";
        default:
          String t = prefs.getString(r.key, r.defText);
          if (t == null || t.isEmpty()) return getString(R.string.default_value);
          return t.replace('\n', ' ');
      }
    }

    @Override
    public View getView(int position, View convert, ViewGroup parent) {
      Row r = rows.get(position);
      LinearLayout line = new LinearLayout(SettingsActivity.this);
      line.setOrientation(LinearLayout.HORIZONTAL);
      line.setPadding(dp(16), dp(12), dp(16), dp(12));
      TextView name = new TextView(SettingsActivity.this);
      name.setTextSize(18);
      name.setText(r.title);
      TextView value = new TextView(SettingsActivity.this);
      value.setTextSize(18);
      value.setGravity(Gravity.END);
      value.setText(valueText(r));
      line.addView(name, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
      line.addView(value, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
          ViewGroup.LayoutParams.WRAP_CONTENT));
      if (!enabled(r)) line.setAlpha(0.4f);
      return line;
    }
  }
}
