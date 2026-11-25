#include "scr.h"
#include "scr_res.h"
#include "../../opts.h"
#include "../../ui/opt_table.h"
#include "../../version.h"
#include "../../xalloc.h"

#include <stdlib.h>
#include <string.h>

#define REG_PATH L"Software\\underthec\\Screensaver"

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

_Static_assert(SCENE_AQUATIC_FLAG_COUNT == SCR_FLAG_SLOTS, "dialog checkboxes out of sync with creatures");

/* shown over visible dialog */
#define WM_LOAD_FAILED (WM_APP + 1)

static HFONT message_font;
static char load_err[256];

static char *utf8_from_wide(const WCHAR *w) {
  int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
  char *s = xmalloc(n > 0 ? (size_t)n : 1);
  if (n <= 0 || WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, NULL, NULL) <= 0) s[0] = '\0';
  return s;
}

static WCHAR *wide_from_utf8(const char *s) {
  int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
  WCHAR *w = xmalloc((n > 0 ? (size_t)n : 1) * sizeof(*w));
  if (n <= 0 || MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n) <= 0) w[0] = L'\0';
  return w;
}

static char *reg_read(HKEY key, const char *name) {
  WCHAR *wname = wide_from_utf8(name);
  DWORD type = 0;
  DWORD size = 0;
  char *out = NULL;
  if (RegQueryValueExW(key, wname, NULL, &type, NULL, &size) == ERROR_SUCCESS && type == REG_SZ) {
    WCHAR *buf = xcalloc(size / sizeof(WCHAR) + 1, sizeof(WCHAR));
    if (RegQueryValueExW(key, wname, NULL, NULL, (BYTE *)buf, &size) == ERROR_SUCCESS) out = utf8_from_wide(buf);
    free(buf);
  }
  free(wname);
  return out;
}

bool scr_config_load(struct config *cfg, char *err, size_t err_len) {
  HKEY key;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &key) != ERROR_SUCCESS) return true;
  bool ok = true;
  for (size_t i = 0; i < OT_COUNT && ok; i++) {
    char *val = reg_read(key, ot_names[i]);
    if (val == NULL) continue;
    ok = config_set(cfg, ot_names[i], val, ot_names[i], err, err_len);
    free(val);
  }
  RegCloseKey(key);
  return ok;
}

/* NULL val: delete */
static bool config_save(char *const vals[OT_COUNT]) {
  HKEY key;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, NULL, 0, KEY_WRITE, NULL, &key, NULL) != ERROR_SUCCESS) return false;
  bool ok = true;
  for (size_t i = 0; i < OT_COUNT; i++) {
    WCHAR *wname = wide_from_utf8(ot_names[i]);
    if (vals[i] == NULL) {
      RegDeleteValueW(key, wname);
    } else {
      WCHAR *wval = wide_from_utf8(vals[i]);
      DWORD size = (DWORD)((wcslen(wval) + 1) * sizeof(WCHAR));
      if (RegSetValueExW(key, wname, 0, REG_SZ, (const BYTE *)wval, size) != ERROR_SUCCESS) ok = false;
      free(wval);
    }
    free(wname);
  }
  RegCloseKey(key);
  return ok;
}

static char *item_text(HWND dlg, int id) {
  HWND ctl = GetDlgItem(dlg, id);
  int len = GetWindowTextLengthW(ctl);
  WCHAR *w = xcalloc((size_t)len + 1, sizeof(WCHAR));
  GetWindowTextW(ctl, w, len + 1);
  char *s = utf8_from_wide(w);
  free(w);
  return s;
}

static void item_set_text(HWND dlg, int id, const char *s) {
  WCHAR *w = wide_from_utf8(s);
  SetDlgItemTextW(dlg, id, w);
  free(w);
}

static void combo_fill(HWND dlg, int id, const char *const *items, size_t count) {
  for (size_t i = 0; i < count; i++) SendDlgItemMessageA(dlg, id, CB_ADDSTRING, 0, (LPARAM)items[i]);
}

/* one-digit modes padded to align with "16" */
static void colors_combo_fill(HWND dlg, int id) {
  for (size_t i = 0; i < OT_COLORS_N; i++) {
    const char *label = ot_colors_labels[i];
    char buf[24];
    size_t p = 0;
    if (label[1] < '0' || label[1] > '9') opts_append_char_bounded(buf, sizeof buf - 1, &p, ' ');
    opts_append_bounded(buf, sizeof buf - 1, &p, label);
    buf[p] = '\0';
    SendDlgItemMessageA(dlg, id, CB_ADDSTRING, 0, (LPARAM)buf);
  }
}

static int combo_sel(HWND dlg, int id) {
  LRESULT sel = SendDlgItemMessageW(dlg, id, CB_GETCURSEL, 0, 0);
  return sel == CB_ERR ? 0 : (int)sel;
}

static bool checked(HWND dlg, int id) { return IsDlgButtonChecked(dlg, id) == BST_CHECKED; }

static void castle_update_enabled(HWND dlg) {
  bool free_mode = combo_sel(dlg, IDC_CLASSIC) == 0;
  EnableWindow(GetDlgItem(dlg, IDC_CASTLE_NAME), free_mode && checked(dlg, IDC_CASTLE_ENABLED));
}

static void update_enabled(HWND dlg) {
  bool free_mode = combo_sel(dlg, IDC_CLASSIC) == 0;
  EnableWindow(GetDlgItem(dlg, IDC_FISH_AUTO), free_mode);
  EnableWindow(GetDlgItem(dlg, IDC_FISH), free_mode && !checked(dlg, IDC_FISH_AUTO));
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++) EnableWindow(GetDlgItem(dlg, IDC_FLAG0 + (int)i), free_mode);
  EnableWindow(GetDlgItem(dlg, IDC_MESSAGE), free_mode);
  EnableWindow(GetDlgItem(dlg, IDC_MSG_COLOR), free_mode);
  EnableWindow(GetDlgItem(dlg, IDC_MSG_POS), free_mode);
  EnableWindow(GetDlgItem(dlg, IDC_TIME), free_mode);
  EnableWindow(GetDlgItem(dlg, IDC_TIME_COLOR), free_mode && combo_sel(dlg, IDC_TIME) != 0);
  EnableWindow(GetDlgItem(dlg, IDC_FPS), free_mode);
  EnableWindow(GetDlgItem(dlg, IDC_UTURN), free_mode);
  EnableWindow(GetDlgItem(dlg, IDC_AUTOFEED), free_mode);
  castle_update_enabled(dlg);
}

/* LF to CRLF for edit control */
static char *crlf_from_lf(const char *s) {
  char *out = xmalloc(strlen(s) * 2 + 1);
  char *o = out;
  for (; *s != '\0'; s++) {
    if (*s == '\n') *o++ = '\r';
    *o++ = *s;
  }
  *o = '\0';
  return out;
}

static void strip_cr(char *s) {
  char *o = s;
  for (; *s != '\0'; s++)
    if (*s != '\r') *o++ = *s;
  *o = '\0';
}

static void controls_from_config(HWND dlg, struct config *cfg) {
  char buf[32];
  SendDlgItemMessageW(dlg, IDC_CLASSIC, CB_SETCURSEL, (WPARAM)cfg->classic_ver, 0);
  bool fish_auto = cfg->aquatic.fish_count < 0;
  CheckDlgButton(dlg, IDC_FISH_AUTO, fish_auto ? BST_CHECKED : BST_UNCHECKED);
  opts_format_int(buf, sizeof buf, fish_auto ? 0 : cfg->aquatic.fish_count);
  item_set_text(dlg, IDC_FISH, fish_auto ? "" : buf);
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++) {
    bool on = *scene_aquatic_flag(&cfg->aquatic, i);
    CheckDlgButton(dlg, IDC_FLAG0 + (int)i, on ? BST_CHECKED : BST_UNCHECKED);
  }
  char *msg = crlf_from_lf(cfg->message != NULL ? cfg->message : "");
  item_set_text(dlg, IDC_MESSAGE, msg);
  free(msg);
  int color = 0;
  for (size_t i = 1; cfg->message_color != NULL && i < COUNT(ot_color_items); i++)
    if (strcmp(cfg->message_color, ot_color_items[i]) == 0) color = (int)i;
  SendDlgItemMessageW(dlg, IDC_MSG_COLOR, CB_SETCURSEL, (WPARAM)color, 0);
  SendDlgItemMessageW(dlg, IDC_MSG_POS, CB_SETCURSEL, (WPARAM)cfg->message_position, 0);
  int time_sel = 0;
  if (cfg->time_mode == 24) time_sel = 1;
  else if (cfg->time_mode == 12) time_sel = 2;
  SendDlgItemMessageW(dlg, IDC_TIME, CB_SETCURSEL, (WPARAM)time_sel, 0);
  int time_color = 0;
  for (size_t i = 1; cfg->time_color != NULL && i < COUNT(ot_color_items); i++)
    if (strcmp(cfg->time_color, ot_color_items[i]) == 0) time_color = (int)i;
  SendDlgItemMessageW(dlg, IDC_TIME_COLOR, CB_SETCURSEL, (WPARAM)time_color, 0);
  CheckDlgButton(dlg, IDC_CASTLE_ENABLED, cfg->no_castle ? BST_UNCHECKED : BST_CHECKED);
  item_set_text(dlg, IDC_CASTLE_NAME, cfg->castle_name != NULL ? cfg->castle_name : "");
  castle_update_enabled(dlg);
  { size_t p = 0; opts_append_float_bounded(buf, sizeof buf - 1, &p, cfg->pace, 0, 2); buf[p] = '\0'; }
  char *end = buf + strlen(buf);
  while (end[-1] == '0') *--end = '\0';
  if (end[-1] == '.') end[-1] = '\0';
  item_set_text(dlg, IDC_PACE, buf);
  opts_format_int(buf, sizeof buf, cfg->fps);
  item_set_text(dlg, IDC_FPS, buf);
  opts_format_int(buf, sizeof buf, cfg->uturn_chance);
  item_set_text(dlg, IDC_UTURN, buf);
  opts_format_int(buf, sizeof buf, cfg->autofeed);
  item_set_text(dlg, IDC_AUTOFEED, buf);
  SendDlgItemMessageW(dlg, IDC_COLORS, CB_SETCURSEL, (WPARAM)ot_colors_index(cfg->colors_mode), 0);
  update_enabled(dlg);
}

static char *aquatic_definition(HWND dlg) {
  char *fish = checked(dlg, IDC_FISH_AUTO) ? opts_strdup("auto") : item_text(dlg, IDC_FISH);
  size_t cap = strlen(fish) + 8;
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++) cap += strlen(scene_aquatic_flag_name(i)) + 1;
  char *def = xmalloc(cap);
  size_t pos = 0;
  opts_append_bounded(def, cap, &pos, "fish=");
  opts_append_bounded(def, cap, &pos, fish);
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++) {
    if (!checked(dlg, IDC_FLAG0 + (int)i)) continue;
    opts_append_bounded(def, cap, &pos, ",");
    opts_append_bounded(def, cap, &pos, scene_aquatic_flag_name(i));
  }
  def[pos] = '\0';
  free(fish);
  return def;
}

static bool apply(HWND dlg) {
  char *vals[OT_COUNT] = {0};
  int classic = combo_sel(dlg, IDC_CLASSIC);
  if (classic > 0) vals[OT_CLASSIC] = opts_strdup(ot_classic_items[classic]);
  else vals[OT_AQUATIC] = aquatic_definition(dlg);
  vals[OT_MESSAGE] = item_text(dlg, IDC_MESSAGE);
  strip_cr(vals[OT_MESSAGE]);
  if (vals[OT_MESSAGE][0] == '\0') {
    free(vals[OT_MESSAGE]);
    vals[OT_MESSAGE] = NULL;
  }
  int color = combo_sel(dlg, IDC_MSG_COLOR);
  if (color > 0) vals[OT_COLOR] = opts_strdup(ot_color_items[color]);
  int time_sel = combo_sel(dlg, IDC_TIME);
  int position = combo_sel(dlg, IDC_MSG_POS);
  if (time_sel > 0) vals[OT_TIME] = opts_strdup(ot_time_items[time_sel]);
  int time_color = combo_sel(dlg, IDC_TIME_COLOR);
  if (time_sel > 0 && time_color > 0) vals[OT_TIME_COLOR] = opts_strdup(ot_color_items[time_color]);
  if (time_sel == 0 || position != 0) vals[OT_POSITION] = opts_strdup(ot_position_items[position]);
  vals[OT_CASTLE_NAME] = item_text(dlg, IDC_CASTLE_NAME);
  if (vals[OT_CASTLE_NAME][0] == '\0') {
    free(vals[OT_CASTLE_NAME]);
    vals[OT_CASTLE_NAME] = NULL;
  }
  vals[OT_NO_CASTLE] = opts_strdup(checked(dlg, IDC_CASTLE_ENABLED) ? "0" : "1");
  vals[OT_PACE] = item_text(dlg, IDC_PACE);
  vals[OT_UTURN] = item_text(dlg, IDC_UTURN);
  vals[OT_FPS] = item_text(dlg, IDC_FPS);
  vals[OT_AUTOFEED] = item_text(dlg, IDC_AUTOFEED);
  if (vals[OT_AUTOFEED][0] == '\0') {
    free(vals[OT_AUTOFEED]);
    vals[OT_AUTOFEED] = NULL;
  }
  vals[OT_COLORS] = opts_strdup(ot_colors_items[combo_sel(dlg, IDC_COLORS)]);

  char err[256];
  struct config cfg;
  config_init(&cfg);
  bool ok = true;
  for (size_t i = 0; i < OT_COUNT; i++) {
    if (vals[i] == NULL) continue;
    ok = config_set(&cfg, ot_names[i], vals[i], ot_labels[i], err, sizeof err);
    if (!ok) break;
  }
  if (ok) ok = config_check(&cfg, err, sizeof err);
  config_free(&cfg);
  if (!ok) {
    WCHAR *w = wide_from_utf8(err);
    MessageBoxW(dlg, w, TOOL_DISPLAY_NAME_W, MB_OK | MB_ICONWARNING);
    free(w);
  } else if (!config_save(vals)) {
    MessageBoxW(dlg, L"Could not save settings to the registry.", TOOL_DISPLAY_NAME_W, MB_OK | MB_ICONERROR);
    ok = false;
  }
  for (size_t i = 0; i < OT_COUNT; i++) free(vals[i]);
  return ok;
}

typedef UINT(WINAPI *dpi_for_window_fn)(HWND);

/* GetDpiForWindow: Win 10 1607+(?) */
static UINT window_dpi(HWND wnd) {
  FARPROC p = GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
  if (p != NULL) return ((dpi_for_window_fn)(void (*)(void))p)(wnd);
  HDC dc = GetDC(wnd);
  UINT dpi = (UINT)GetDeviceCaps(dc, LOGPIXELSY);
  ReleaseDC(wnd, dc);
  return dpi;
}

/* WM_SETFONT fonts not rescaled by dialog manager */
static void message_font_set(HWND dlg, UINT dpi) {
  HFONT old = message_font;
  message_font = CreateFontW(-MulDiv(9, (int)dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
  SendDlgItemMessageW(dlg, IDC_MESSAGE, WM_SETFONT, (WPARAM)message_font, TRUE);
  if (old != NULL) DeleteObject(old);
}

static void init_dialog(HWND dlg) {
  combo_fill(dlg, IDC_CLASSIC, ot_classic_items, COUNT(ot_classic_items));
  combo_fill(dlg, IDC_MSG_COLOR, ot_color_labels, COUNT(ot_color_labels));
  combo_fill(dlg, IDC_MSG_POS, ot_position_items, COUNT(ot_position_items));
  combo_fill(dlg, IDC_TIME, ot_time_labels, COUNT(ot_time_labels));
  combo_fill(dlg, IDC_TIME_COLOR, ot_color_labels, COUNT(ot_color_labels));
  colors_combo_fill(dlg, IDC_COLORS);
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++) item_set_text(dlg, IDC_FLAG0 + (int)i, scene_aquatic_flag_name(i));
  message_font_set(dlg, window_dpi(dlg));
  item_set_text(dlg, IDC_VERSION, TOOL_DISPLAY_NAME " v" TOOL_VERSION);

  struct config cfg;
  config_init(&cfg);
  if (!scr_config_load(&cfg, load_err, sizeof load_err)) {
    config_free(&cfg);
    config_init(&cfg);
    PostMessageW(dlg, WM_LOAD_FAILED, 0, 0);
  }
  controls_from_config(dlg, &cfg);
  config_free(&cfg);
}

static void show_load_error(HWND dlg) {
  ShowWindow(dlg, SW_SHOW);
  UpdateWindow(dlg);
  char msg[320];
  opts_set_errbuf(msg, sizeof msg, (const char *[]){"Stored settings are invalid, showing defaults.\n\n", load_err}, 2);
  WCHAR *w = wide_from_utf8(msg);
  MessageBoxW(dlg, w, TOOL_DISPLAY_NAME_W, MB_OK | MB_ICONWARNING);
  free(w);
}

static INT_PTR CALLBACK dlg_proc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_INITDIALOG:
      init_dialog(dlg);
      return TRUE;
    case WM_DPICHANGED:
      message_font_set(dlg, HIWORD(wp));
      return FALSE;
    case WM_LOAD_FAILED:
      show_load_error(dlg);
      return TRUE;
    case WM_CTLCOLORSTATIC:
      if ((HWND)lp == GetDlgItem(dlg, IDC_VERSION)) {
        SetTextColor((HDC)wp, GetSysColor(COLOR_GRAYTEXT));
        SetBkMode((HDC)wp, TRANSPARENT);
        return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
      }
      return FALSE;
    case WM_COMMAND:
      switch (LOWORD(wp)) {
        case IDC_CLASSIC:
          if (HIWORD(wp) == CBN_SELCHANGE) update_enabled(dlg);
          return TRUE;
        case IDC_TIME:
          if (HIWORD(wp) == CBN_SELCHANGE) update_enabled(dlg);
          return TRUE;
        case IDC_FISH_AUTO:
          update_enabled(dlg);
          return TRUE;
        case IDC_CASTLE_ENABLED:
          castle_update_enabled(dlg);
          return TRUE;
        case IDC_DEFAULTS: {
          struct config cfg;
          config_init(&cfg);
          controls_from_config(dlg, &cfg);
          config_free(&cfg);
          return TRUE;
        }
        case IDOK:
          if (apply(dlg)) EndDialog(dlg, IDOK);
          return TRUE;
        case IDCANCEL:
          EndDialog(dlg, IDCANCEL);
          return TRUE;
        default:
          return FALSE;
      }
    case WM_DESTROY:
      if (message_font != NULL) DeleteObject(message_font);
      message_font = NULL;
      return FALSE;
    default:
      return FALSE;
  }
}

void scr_config_dialog(HINSTANCE inst, HWND parent) {
  DialogBoxParamW(inst, MAKEINTRESOURCEW(IDD_CONFIG), parent, dlg_proc, 0);
}
