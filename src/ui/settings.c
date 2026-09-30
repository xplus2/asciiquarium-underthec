#include "settings.h"
#include "../color.h"
#include "../opts.h"
#include "../target/terminal/term.h"

#include <stddef.h>
#include <string.h>

enum field_kind { FIELD_NONE, FIELD_FPS, FIELD_PACE, FIELD_UTURN, FIELD_COLORS, FIELD_FISH, FIELD_SPECIES, FIELD_CASTLE };

static bool is_checkbox_kind(enum field_kind k) { return k == FIELD_SPECIES || k == FIELD_CASTLE; }

struct grid_cell {
  enum field_kind kind;
  const char *label;
  size_t species_offset;
};

#define SP(field) offsetof(struct aquatic_life, field)

#define GRID_ROWS 13

static const struct grid_cell grid[GRID_ROWS][2] = {
{{FIELD_COLORS, "colors", 0},                 {FIELD_NONE, NULL, 0}},
{{FIELD_FPS, "fps", 0},                       {FIELD_PACE, "pace", 0}},
{{FIELD_UTURN, "uturn", 0},                   {FIELD_FISH, "fish", 0}},
{{FIELD_SPECIES, "bigfish", SP(bigfish)},     {FIELD_SPECIES, "sailboat", SP(sailboat)}},
{{FIELD_SPECIES, "crab", SP(crab)},           {FIELD_SPECIES, "seahorse", SP(seahorse)}},
{{FIELD_SPECIES, "dolphins", SP(dolphins)},   {FIELD_SPECIES, "shark", SP(shark)}},
{{FIELD_SPECIES, "ducks", SP(ducks)},         {FIELD_SPECIES, "ship", SP(ship)}},
{{FIELD_SPECIES, "fishhook", SP(fishhook)},   {FIELD_SPECIES, "submarine", SP(submarine)}},
{{FIELD_SPECIES, "jellyfish", SP(jellyfish)}, {FIELD_SPECIES, "swan", SP(swan)}},
{{FIELD_SPECIES, "kaiju", SP(kaiju)},         {FIELD_SPECIES, "swordfish", SP(swordfish)}},
{{FIELD_SPECIES, "monster", SP(monster)},     {FIELD_SPECIES, "turtle", SP(turtle)}},
{{FIELD_SPECIES, "rowers", SP(rowers)},       {FIELD_SPECIES, "whale", SP(whale)}},
{{FIELD_CASTLE, "castle", 0},                 {FIELD_NONE, NULL, 0}},
};

#undef SP

#define CONTENT_W 28
#define CONTENT_H 15
#define MARGIN 1
#define BOX_W (CONTENT_W + 2 * MARGIN)
#define BOX_H (CONTENT_H + 2 * MARGIN)
#define COL0_X 0
#define COL1_X 14
#define MINUS0_X 7
#define PLUS0_X 11
#define MINUS1_X 20
#define PLUS1_X 25
#define PLUS_COLORS_X 25
#define PLUS1_FISH_X 25
/* "[x] " before species label */
#define CHECK_PREFIX 4

static const struct attr BOX_ATTR = {.col = COL_WHITE, .bold = false, .bg = COL_BLACK, .bg_bold = true};
static const struct attr HL_ATTR = {.col = COL_YELLOW, .bold = false, .bg = COL_BLUE, .bg_bold = false};

static int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static const int COLORS_MODES[] = {1, 11, 21, 31, 41, 51, 61, 71, 2, 12, 22, 32, 42, 52, 62, 72, 4, 7, 8, 108, 16};
#define COLORS_MODES_N (int)(sizeof(COLORS_MODES) / sizeof(COLORS_MODES[0]))

static int colors_mode_index(int mode) {
  for (int i = 0; i < COLORS_MODES_N; i++) if (COLORS_MODES[i] == mode) return i;
  return COLORS_MODES_N - 1;
}

static void colors_mode_label(int mode, char *out, size_t out_cap) {
  int base;
  int accent_id;
  color_mode_decode(mode, &base, &accent_id);
  if (accent_id != 0) {
    size_t p = 0;
    opts_append_bounded(out, out_cap - 1, &p, "  ");
    opts_append_int_bounded(out, out_cap - 1, &p, base, 0);
    opts_append_bounded(out, out_cap - 1, &p, "-");
    opts_append_bounded(out, out_cap - 1, &p, color_accent_names[accent_id - 1]);
    out[p] = '\0';
    return;
  }
  if (mode == 4) { memcpy(out, "  4 (RGBW)", sizeof "  4 (RGBW)"); return; }
  if (mode == 7) { memcpy(out, "  7 (Teletext)", sizeof "  7 (Teletext)"); return; }
  if (mode == 108) { memcpy(out, "  8-bold", sizeof "  8-bold"); return; }
  if (mode == 16) { memcpy(out, " 16 (ANSI)", sizeof " 16 (ANSI)"); return; }
  { size_t p = 0; opts_append_bounded(out, out_cap - 1, &p, "  "); opts_append_int_bounded(out, out_cap - 1, &p, mode, 0); out[p] = '\0'; }
}

static double clampd(double v, double lo, double hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static bool *species_field(struct aquatic_life *a, size_t offset) {
  return (bool *)((char *)a + offset);
}

static int current_fish_value(const struct scene *sc) {
  return sc->aquatic.fish_count >= 0 ? sc->aquatic.fish_count : scene_fish_display_count(sc);
}

static int draw_row_for_logical(int lr) { return lr + 2; }

static int logical_for_draw_row(int dr) {
  for (int lr = 0; lr < GRID_ROWS; lr++)
    if (draw_row_for_logical(lr) == dr) return lr;
  return -1;
}

void settings_ui_init(struct settings_ui *ui, int *fps, double *pace, int *colors_mode, struct scene *scene) {
  ui->open = false;
  ui->fps = fps;
  ui->pace = pace;
  ui->colors_mode = colors_mode;
  ui->scene = scene;
  ui->sel_row = 0;
  ui->sel_col = 0;
}

bool settings_ui_is_open(const struct settings_ui *ui) { return ui->open; }

void settings_ui_toggle(struct settings_ui *ui) {
  ui->open = !ui->open;
  if (ui->open) {
    ui->sel_row = 0;
    ui->sel_col = 0;
  }
}

void settings_ui_close(struct settings_ui *ui) { ui->open = false; }

static void move_left_right(struct settings_ui *ui, int dcol) {
  int other = ui->sel_col + dcol;
  if (other < 0 || other > 1) return;
  if (grid[ui->sel_row][other].kind == FIELD_NONE) return;
  ui->sel_col = other;
}

static void move_up_down(struct settings_ui *ui, int drow) {
  int row = clampi(ui->sel_row + drow, 0, GRID_ROWS - 1);
  int col = ui->sel_col;
  if (grid[row][col].kind == FIELD_NONE) col = 0;
  ui->sel_row = row;
  ui->sel_col = col;
}

static void adjust(struct settings_ui *ui, int dir, int term_w, int term_h) {
  const struct grid_cell *cell = &grid[ui->sel_row][ui->sel_col];
  switch (cell->kind) {
    case FIELD_FPS:
      *ui->fps = clampi(*ui->fps + dir, 1, 240);
      break;
    case FIELD_PACE: {
      int tenths = (int)(*ui->pace * 10.0 + (*ui->pace >= 0 ? 0.5 : -0.5)) + dir;
      *ui->pace = clampd((double)tenths / 10.0, 0.01, 10.0);
      break;
    }
    case FIELD_UTURN:
      scene_set_uturn_chance(ui->scene, clampi(ui->scene->uturn_chance + dir, 0, 999));
      break;
    case FIELD_COLORS: {
      int idx = (colors_mode_index(*ui->colors_mode) + dir + COLORS_MODES_N) % COLORS_MODES_N;
      *ui->colors_mode = COLORS_MODES[idx];
      break;
    }
    case FIELD_FISH: {
      int next = clampi(current_fish_value(ui->scene) + dir, 0, 999);
      scene_set_fish_count(ui->scene, term_w, term_h, next);
      break;
    }
    case FIELD_SPECIES:
    case FIELD_NONE:
    default: break;
  }
}

static void toggle_checkbox(struct settings_ui *ui, int term_w, int term_h) {
  const struct grid_cell *cell = &grid[ui->sel_row][ui->sel_col];
  if (cell->kind == FIELD_SPECIES) {
    bool *flag = species_field(&ui->scene->aquatic, cell->species_offset);
    *flag = !*flag;
    scene_on_species_toggled(ui->scene, term_w, term_h);
  } else if (cell->kind == FIELD_CASTLE) {
    scene_toggle_castle(ui->scene, term_w, term_h);
  }
}

void settings_ui_handle_key(struct settings_ui *ui, int key, int term_w, int term_h) {
  if (!ui->open) return;
  switch (key) {
    case TERM_KEY_UP:    move_up_down(ui, -1);           break;
    case TERM_KEY_DOWN:  move_up_down(ui, 1);            break;
    case TERM_KEY_LEFT:  move_left_right(ui, -1);        break;
    case TERM_KEY_RIGHT: move_left_right(ui, 1);         break;
    case '+':            adjust(ui, 1, term_w, term_h);    break;
    case '-':            adjust(ui, -1, term_w, term_h);   break;
    case ' ':            toggle_checkbox(ui, term_w, term_h); break;
    default: break;
  }
}

bool settings_ui_click(struct settings_ui *ui, int x, int y, int term_w, int term_h) {
  if (!ui->open || x < 0 || x >= BOX_W || y < 0 || y >= BOX_H) return false;
  int lr = logical_for_draw_row(y - MARGIN);
  if (lr < 0) return true;
  int cx = x - MARGIN;
  int col;
  int dir = 0;
  if (!is_checkbox_kind(grid[lr][0].kind)) {
    int plus0_x = grid[lr][0].kind == FIELD_COLORS ? PLUS_COLORS_X : PLUS0_X;
    int plus1_x = grid[lr][1].kind == FIELD_FISH ? PLUS1_FISH_X : PLUS1_X;
    if (cx == MINUS0_X || cx == plus0_x) col = 0;
    else if (cx == MINUS1_X || cx == plus1_x) col = 1;
    else return true;
    if (grid[lr][col].kind == FIELD_NONE) return true;
    dir = (cx == plus0_x || cx == plus1_x) ? 1 : -1;
  } else {
    col = cx >= COL1_X ? 1 : 0;
    const struct grid_cell *cell = &grid[lr][col];
    int x0 = col == 0 ? COL0_X : COL1_X;
    if (!is_checkbox_kind(cell->kind) || cx < x0 || cx >= x0 + CHECK_PREFIX + (int)strlen(cell->label)) return true;
  }
  ui->sel_row = lr;
  ui->sel_col = col;
  if (dir != 0) adjust(ui, dir, term_w, term_h);
  else toggle_checkbox(ui, term_w, term_h);
  return true;
}

static void fill_rect(struct canvas *c, int x0, int y0, int w, int h, struct attr a) {
  for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) canvas_put(c, x0 + x, y0 + y, " ", 1, a, 1);
}

static void draw_row_text(struct canvas *c, int x0, int y, const char *s, int hl_start, int hl_len) {
  for (int i = 0; s[i] != '\0'; i++) {
    struct attr a = (hl_len > 0 && i >= hl_start && i < hl_start + hl_len) ? HL_ATTR : BOX_ATTR;
    char g[2] = {s[i], '\0'};
    canvas_put(c, x0 + i, y, g, 1, a, 1);
  }
}

struct adj_field {
  enum field_kind kind;
  const char *label;
  int label_w;
  int value_w;
};

static const struct adj_field TOP_ROWS[3][2] = {
  {{FIELD_COLORS, "colors", 7, 17}, {FIELD_NONE, NULL, 0, 0}},
  {{FIELD_FPS, "fps", 7, 3}, {FIELD_PACE, "pace", 6, 4}},
  {{FIELD_UTURN, "uturn", 7, 3}, {FIELD_FISH, "fish", 6, 4}},
};

static void format_adj_value(const struct settings_ui *ui, enum field_kind kind, int value_w, char *out, size_t out_cap) {
  size_t p = 0;
  switch (kind) {
    case FIELD_FPS: opts_append_int_bounded(out, out_cap - 1, &p, *ui->fps, value_w); break;
    case FIELD_PACE: opts_append_float_bounded(out, out_cap - 1, &p, *ui->pace, value_w, 1); break;
    case FIELD_UTURN: opts_append_int_bounded(out, out_cap - 1, &p, ui->scene->uturn_chance, value_w); break;
    case FIELD_FISH: opts_append_int_bounded(out, out_cap - 1, &p, current_fish_value(ui->scene), value_w); break;
    case FIELD_COLORS: {
      char lbl[15];
      colors_mode_label(*ui->colors_mode, lbl, sizeof lbl);
      opts_append_bounded_w(out, out_cap - 1, &p, lbl, value_w);
      break;
    }
    case FIELD_SPECIES:
    case FIELD_CASTLE:
    case FIELD_NONE:
    default: break;
  }
  out[p] = '\0';
}

static void draw_adjustable_row(const struct settings_ui *ui, struct canvas *c, int lr, const struct adj_field row[2]) {
  char line[CONTENT_W + 1];
  size_t p = 0;
  bool sel = ui->sel_row == lr;
  int hl_off = -1;
  int hl_len = 0;
  for (int col = 0; col < 2; col++) {
    if (row[col].kind == FIELD_NONE) break;
    if (col == 1) opts_append_bounded(line, sizeof line - 1, &p, "  ");
    opts_append_bounded_w(line, sizeof line - 1, &p, row[col].label, row[col].label_w);
    opts_append_bounded(line, sizeof line - 1, &p, "-");
    size_t value_start = p;
    char valbuf[20];
    format_adj_value(ui, row[col].kind, row[col].value_w, valbuf, sizeof valbuf);
    opts_append_bounded(line, sizeof line - 1, &p, valbuf);
    if (sel && ui->sel_col == col) { hl_off = (int)value_start; hl_len = (int)(p - value_start); }
    opts_append_bounded(line, sizeof line - 1, &p, "+");
  }
  line[p] = '\0';
  draw_row_text(c, MARGIN, MARGIN + draw_row_for_logical(lr), line, hl_off, hl_len);
}

void settings_ui_draw(const struct settings_ui *ui, struct canvas *c) {
  if (!ui->open) return;
  fill_rect(c, 0, 0, BOX_W, BOX_H, BOX_ATTR);
  draw_row_text(c, MARGIN, MARGIN + 0, "Settings", -1, 0);
  for (int lr = 0; lr < 3; lr++) draw_adjustable_row(ui, c, lr, TOP_ROWS[lr]);
  for (int lr = 3; lr < GRID_ROWS; lr++) {
    int y = MARGIN + draw_row_for_logical(lr);
    for (int col = 0; col < 2; col++) {
      const struct grid_cell *cell = &grid[lr][col];
      if (cell->kind == FIELD_NONE) continue;
      bool on = cell->kind == FIELD_CASTLE ? ui->scene->castle : *species_field(&ui->scene->aquatic, cell->species_offset);
      char buf[16];
      {
        size_t p = 0;
        opts_append_bounded(buf, sizeof buf - 1, &p, "[");
        opts_append_char_bounded(buf, sizeof buf - 1, &p, on ? 'x' : ' ');
        opts_append_bounded(buf, sizeof buf - 1, &p, "] ");
        opts_append_bounded(buf, sizeof buf - 1, &p, cell->label);
        buf[p] = '\0';
      }
      bool sel = ui->sel_row == lr && ui->sel_col == col;
      draw_row_text(c, MARGIN + (col == 0 ? COL0_X : COL1_X), y, buf, sel ? 0 : -1, sel ? 3 : 0);
    }
  }
}
