package org.underthec;

import android.content.Context;
import android.content.SharedPreferences;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

final class Options {
  static final String[] CREATURES = {
      "bigfish", "crab", "dolphins", "ducks", "fishhook", "jellyfish", "kaiju", "monster", "rowers",
      "sailboat", "seahorse", "shark", "ship", "submarine", "swan", "swordfish", "turtle", "whale"
  };
  static final String[] MESSAGE_COLORS = {
      "", "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white",
      "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White"
  };
  static final String[] POSITIONS = {"middle", "center", "marquee", "swim", "event"};
  static final String[] COLORS = {
      "1", "1-green", "1-red", "1-blue", "1-yellow", "1-magenta", "1-cyan", "1-white",
      "2", "2-green", "2-red", "2-blue", "2-yellow", "2-magenta", "2-cyan", "2-white",
      "4", "7", "8", "8-bold", "16"
  };
  static final String[] COLORS_LABELS = {
      "1", "1-green", "1-red", "1-blue", "1-yellow", "1-magenta", "1-cyan", "1-white",
      "2", "2-green", "2-red", "2-blue", "2-yellow", "2-magenta", "2-cyan", "2-white",
      "4 (RGBW)", "7 (Teletext)", "8", "8-bold", "16 (ANSI)"
  };

  static final String DEFAULT_LIFE;

  static {
    StringBuilder b = new StringBuilder("fish=auto");
    for (String c : CREATURES) b.append(',').append(c);
    DEFAULT_LIFE = b.toString();
  }

  private Options() {
  }

  static SharedPreferences prefs(Context c) {
    return c.getSharedPreferences("underthec", Context.MODE_PRIVATE);
  }

  /* --aquatic-life */
  static final class Life {
    boolean auto = true;
    int fish = 20;
    final Set<String> creatures = new LinkedHashSet<>();

    static Life parse(String s) {
      Life l = new Life();
      for (String tok : s.split(",")) {
        String t = tok.trim();
        if (t.startsWith("fish=")) {
          String v = t.substring(5);
          if (v.equals("auto")) {
            l.auto = true;
          } else {
            try {
              l.fish = Integer.parseInt(v);
              l.auto = false;
            } catch (NumberFormatException ignored) {
              l.auto = true;
            }
          }
        } else if (!t.isEmpty()) {
          l.creatures.add(t);
        }
      }
      return l;
    }

    @Override
    public String toString() {
      StringBuilder b = new StringBuilder("fish=").append(auto ? "auto" : String.valueOf(fish));
      for (String c : CREATURES) {
        if (creatures.contains(c)) b.append(',').append(c);
      }
      return b.toString();
    }
  }

  /* name/value pairs Native.create */
  static byte[][] build(SharedPreferences p) {
    List<String> o = new ArrayList<>();
    String classic = p.getString("classic", "");
    if (!classic.isEmpty()) add(o, "classic", classic);
    else add(o, "aquatic-life", p.getString("aquatic-life", DEFAULT_LIFE));
    add(o, "message", p.getString("message", ""));
    add(o, "message-color", p.getString("message-color", ""));
    add(o, "message-position", p.getString("message-position", "middle"));
    add(o, "castle-name", p.getString("castle-name", ""));
    add(o, "no-castle", p.getString("no-castle", "0"));
    add(o, "pace", p.getString("pace", "1.00"));
    add(o, "uturn-chance", p.getString("uturn-chance", "400"));
    add(o, "fps", p.getString("fps", "10"));
    add(o, "colors", p.getString("colors", "16"));
    byte[][] out = new byte[o.size()][];
    for (int i = 0; i < out.length; i++) out[i] = o.get(i).getBytes(StandardCharsets.UTF_8);
    return out;
  }

  private static void add(List<String> o, String name, String value) {
    if (value == null || value.isEmpty()) return;
    o.add(name);
    o.add(value);
  }

  static int fontSize(SharedPreferences p) {
    return p.getInt("font-size", 0);
  }

  static String validate(SharedPreferences p) {
    try {
      Native.destroy(Native.create(build(p), System.nanoTime() / 1e9));
      return null;
    } catch (IllegalArgumentException e) {
      return e.getMessage();
    }
  }
}
