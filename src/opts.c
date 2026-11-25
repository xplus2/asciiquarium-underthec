#include "opts.h"
#include "color.h"
#include "utf8.h"
#include "xalloc.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

void opts_append_bounded(char *dst, size_t dst_cap, size_t *pos, const char *src) {
  size_t src_len = strlen(src);
  size_t avail = dst_cap > *pos ? dst_cap - *pos : 0;
  if (src_len > avail) src_len = avail;
  memcpy(dst + *pos, src, src_len);
  *pos += src_len;
}

void opts_append_char_bounded(char *dst, size_t dst_cap, size_t *pos, char c) {
  if (*pos < dst_cap) dst[(*pos)++] = c;
}

void opts_append_bounded_w(char *dst, size_t dst_cap, size_t *pos, const char *src, int width) {
  size_t start = *pos;
  opts_append_bounded(dst, dst_cap, pos, src);
  int written = (int)(*pos - start);
  for (int i = written; i < width; i++) opts_append_char_bounded(dst, dst_cap, pos, ' ');
}

static int uint_to_digits(unsigned long v, char *tmp) {
  int n = 0;
  do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v != 0);
  return n;
}

void opts_append_int_bounded(char *dst, size_t dst_cap, size_t *pos, long v, int width) {
  bool neg = v < 0;
  unsigned long uv = neg ? (unsigned long)(-v) : (unsigned long)v;
  char tmp[24];
  int n = uint_to_digits(uv, tmp);
  int total = n + (neg ? 1 : 0);
  for (int i = total; i < width; i++) opts_append_char_bounded(dst, dst_cap, pos, ' ');
  if (neg) opts_append_char_bounded(dst, dst_cap, pos, '-');
  while (n > 0) opts_append_char_bounded(dst, dst_cap, pos, tmp[--n]);
}

void opts_append_float_bounded(char *dst, size_t dst_cap, size_t *pos, double v, int width, int decimals) {
  bool neg = v < 0;
  if (neg) v = -v;
  unsigned long scale = 1;
  for (int i = 0; i < decimals; i++) scale *= 10;
  unsigned long scaled = (unsigned long)(v * (double)scale + 0.5);
  unsigned long whole = scaled / scale;
  unsigned long frac = scaled % scale;
  char body[32];
  size_t blen = 0;
  if (neg) body[blen++] = '-';
  char tmp[24];
  int n = uint_to_digits(whole, tmp);
  while (n > 0) body[blen++] = tmp[--n];
  if (decimals > 0) {
    body[blen++] = '.';
    n = uint_to_digits(frac, tmp);
    for (int i = n; i < decimals; i++) body[blen++] = '0';
    while (n > 0) body[blen++] = tmp[--n];
  }
  body[blen] = '\0';
  for (int i = (int)blen; i < width; i++) opts_append_char_bounded(dst, dst_cap, pos, ' ');
  opts_append_bounded(dst, dst_cap, pos, body);
}

void opts_format_int(char *out, size_t out_cap, long v) {
  size_t p = 0;
  opts_append_int_bounded(out, out_cap - 1, &p, v, 0);
  out[p] = '\0';
}

void opts_set_errbuf(char *errbuf, size_t errbuf_len, const char *const *parts, size_t count) {
  if (errbuf_len == 0) return;
  size_t pos = 0;
  for (size_t i = 0; i < count; i++) opts_append_bounded(errbuf, errbuf_len - 1, &pos, parts[i]);
  errbuf[pos] = '\0';
}

char *opts_strdup(const char *s) {
  size_t len = strlen(s);
  char *p = xmalloc(len + 1);
  memcpy(p, s, len + 1);
  return p;
}

bool opts_parse_bool(const char *val, bool *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "0") == 0) {
    *out = false;
    return true;
  }
  if (strcmp(val, "1") == 0) {
    *out = true;
    return true;
  }
  opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid value '", val, "', expected 0 or 1"}, 3);
  return false;
}

bool opts_parse_int_range(const char *val, int lo, int hi, int *out) {
  char *end = NULL;
  long n = strtol(val, &end, 10);
  if (val[0] == '\0' || *end != '\0' || n < lo || n > hi) return false;
  *out = (int)n;
  return true;
}

bool opts_parse_fish_count(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "auto") == 0) {
    *out = -1;
    return true;
  }
  if (!opts_parse_int_range(val, 0, OPTS_COUNT_MAX, out)) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid fish count '", val, "', expected 0-999 or auto"}, 3);
    return false;
  }
  return true;
}

bool opts_parse_aquatic_life(const char *definition, struct aquatic_life *out, bool *fish_set, char *errbuf, size_t errbuf_len) {
  struct aquatic_life tmp = *out;
  bool fish_seen = false;
  scene_aquatic_fill(&tmp, false);
  char *buf = opts_strdup(definition);
  size_t len = strlen(buf);
  bool ok = true;
  const char *token = buf;
  for (size_t i = 0; i <= len && ok; i++) {
    if (buf[i] != ',' && buf[i] != '\0') continue;
    buf[i] = '\0';
    if (token[0] == '\0') {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"empty entry in aquatic-life definition"}, 1);
      ok = false;
    } else if (strncmp(token, "fish=", 5) == 0) {
      if (!opts_parse_fish_count(token + 5, &tmp.fish_count, errbuf, errbuf_len)) {
        ok = false;
      } else {
        fish_seen = true;
      }
    } else if (!scene_aquatic_set_flag(&tmp, token)) {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"unknown aquatic-life entry '", token, "'"}, 3);
      ok = false;
    }
    token = buf + i + 1;
  }
  free(buf);
  if (ok) {
    *out = tmp;
    if (fish_seen) *fish_set = true;
  }
  return ok;
}

bool opts_parse_classic(const char *val, int *out_ver, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "0") == 0 || strcmp(val, "off") == 0) {
    *out_ver = 0;
    return true;
  }
  if (val[0] == '\0' || strcmp(val, "1.0") == 0) {
    *out_ver = 1;
    return true;
  }
  if (strcmp(val, "1.1") == 0) {
    *out_ver = 2;
    return true;
  }
  opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid value '", val, "', expected 0, off, 1.0 or 1.1"}, 3);
  return false;
}

bool opts_parse_time(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  if (val[0] == '\0' || strcmp(val, "24") == 0) *out = 24;
  else if (strcmp(val, "12") == 0) *out = 12;
  else {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid time format '", val, "' (expected 24 or 12)"}, 3);
    return false;
  }
  return true;
}

static const char *const message_position_names[] = {
  [MSG_POS_MIDDLE] = "middle", [MSG_POS_CENTER] = "center", [MSG_POS_MARQUEE] = "marquee",
  [MSG_POS_SWIM] = "swim",     [MSG_POS_EVENT] = "event",
};

bool opts_parse_message_position(const char *val, enum message_position *out, char *errbuf, size_t errbuf_len) {
  for (size_t i = 0; i < sizeof(message_position_names) / sizeof(message_position_names[0]); i++) {
    if (strcmp(val, message_position_names[i]) == 0) {
      *out = (enum message_position)i;
      return true;
    }
  }
  opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid message position '", val, "'"}, 3);
  return false;
}

const char *opts_message_position_name(enum message_position pos) {
  return message_position_names[pos];
}

bool opts_parse_uturn_chance(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  if (!opts_parse_int_range(val, 0, OPTS_COUNT_MAX, out)) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid uturn chance '", val, "', expected 0-999"}, 3);
    return false;
  }
  return true;
}

bool opts_parse_autofeed(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  if (val[0] == '\0') {
    *out = AUTOFEED_DEFAULT;
    return true;
  }
  if (!opts_parse_int_range(val, 0, AUTOFEED_MAX, out)) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid autofeed '", val, "', expected 0-99999"}, 3);
    return false;
  }
  return true;
}

bool opts_parse_fps(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  if (!opts_parse_int_range(val, OPTS_FPS_MIN, OPTS_FPS_MAX, out)) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid fps '", val, "', expected 1-240"}, 3);
    return false;
  }
  return true;
}

bool opts_parse_colors(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  const char *dash = strchr(val, '-');
  if (dash != NULL) {
    bool base_ok = dash == val + 1 && (val[0] == '1' || val[0] == '2' || val[0] == '8');
    if (base_ok && val[0] == '8' && strcasecmp(dash + 1, "bold") == 0) {
      *out = 108;
      return true;
    }
    if (!base_ok || val[0] == '8') {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"colors accent '", val, "' only valid for 1, 2 or 8-bold"}, 3);
      return false;
    }
    int accent_id = 0;
    for (int i = 0; i < COLOR_ACCENT_COUNT; i++) {
      if (strcasecmp(dash + 1, color_accent_names[i]) == 0) {
        accent_id = i + 1;
        break;
      }
    }
    if (accent_id == 0) {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid colors accent '", dash + 1, "'"}, 3);
      return false;
    }
    *out = (val[0] - '0') + accent_id * 10;
    return true;
  }
  char *endptr = NULL;
  long n = strtol(val, &endptr, 10);
  if (val[0] == '\0' || *endptr != '\0' || (n != 1 && n != 2 && n != 4 && n != 7 && n != 8 && n != 16)) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid colors '", val, "', expected 1, 2, 4, 7, 8, 8-bold or 16"}, 3);
    return false;
  }
  *out = (int)n;
  return true;
}

void opts_format_colors(int mode, char *out, size_t out_cap) {
  size_t p = 0;
  int base;
  int accent_id;
  color_mode_decode(mode, &base, &accent_id);
  if (mode == 108) {
    opts_append_bounded(out, out_cap - 1, &p, "8-bold");
  } else {
    opts_append_int_bounded(out, out_cap - 1, &p, base, 0);
    if (accent_id != 0) {
      opts_append_char_bounded(out, out_cap - 1, &p, '-');
      opts_append_bounded(out, out_cap - 1, &p, color_accent_names[accent_id - 1]);
    }
  }
  out[p] = '\0';
}

bool opts_parse_pace(const char *s, double *out, char *errbuf, size_t errbuf_len) {
  size_t dot_count = 0;
  bool bad_char = false;
  for (const char *p = s; *p != '\0'; p++) {
    if (*p == '.') dot_count++;
    else if (*p < '0' || *p > '9') bad_char = true;
  }
  char *endptr = NULL;
  double val = strtod(s, &endptr);
  if (bad_char || dot_count > 1 || s[0] == '\0' || *endptr != '\0') {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid pace '", s, "'"}, 3);
    return false;
  }
  const char *dot = strchr(s, '.');
  if (dot != NULL && strlen(dot + 1) > 2) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"pace '", s, "' has more than 2 decimal digits"}, 3);
    return false;
  }
  if (val < OPTS_PACE_MIN - 1e-9 || val > OPTS_PACE_MAX + 1e-9) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"pace '", s, "' out of range 0.01-10"}, 3);
    return false;
  }
  *out = val;
  return true;
}

bool opts_parse_teletext_caption(const char *val, char *out, size_t out_cap, char *errbuf, size_t errbuf_len) {
  size_t len = strlen(val);
  if (len >= out_cap) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"teletext caption '", val, "' too long, max 32 chars"}, 3);
    return false;
  }
  for (size_t i = 0; i < len; i++) {
    if ((unsigned char)val[i] < 0x20 || (unsigned char)val[i] > 0x7E) {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"teletext caption '", val, "' must be printable ASCII"}, 3);
      return false;
    }
  }
  memcpy(out, val, len + 1);
  return true;
}

bool opts_parse_castle_name(const char *val, char *out, size_t out_cap, char *errbuf, size_t errbuf_len) {
  size_t len = strlen(val);
  for (size_t i = 0; i < len; ) {
    unsigned cp;
    int n = utf8_decode_strict(val + i, len - i, &cp);
    if (n < 0) {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"castle name '", val, "' has invalid UTF-8 encoding"}, 3);
      return false;
    }
    if (cp < 0x20 || cp == 0x7F || (cp >= 0x80 && cp <= 0x9F)) {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"castle name '", val, "' must be printable text"}, 3);
      return false;
    }
    i += (size_t)n;
  }
  if (utf8_col_width(val) > CASTLE_NAME_LEN) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"castle name '", val, "' too long, max 11 columns"}, 3);
    return false;
  }
  if (len >= out_cap) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"castle name '", val, "' too long"}, 3);
    return false;
  }
  memcpy(out, val, len + 1);
  return true;
}

/* r past ESC */
static const char *skip_escape(const char *r) {
  if (*r == '[') {
    r++;
    while (*r && (unsigned char)*r < 0x40) r++;
  } else if (*r == ']') {
    r++;
    while (*r && *r != '\a' && !(*r == 0x1B && r[1] == '\\')) r++;
    if (*r == 0x1B) r++;
  }
  return *r ? r + 1 : r;
}

static const char *copy_utf8(char **w, const char *r, const char *end) {
  unsigned cp;
  int n = utf8_decode_strict(r, (size_t)(end - r), &cp);
  if (n < 0) {
    *(*w)++ = '?';
    return r + 1;
  }
  if (cp <= 0x9F) return r + n; /* C1 control */
  memcpy(*w, r, (size_t)n);
  *w += n;
  return r + n;
}

static void opts_sanitize_row(char *row) {
  char *w = row;
  const char *end = row + strlen(row);
  for (const char *r = row; *r;) {
    unsigned char c = (unsigned char)*r;
    if (c == 0x1B) {
      r = skip_escape(r + 1);
    } else if (c == '\t') {
      *w++ = ' ';
      r++;
    } else if (c < 0x20 || c == 0x7F) {
      r++;
    } else if (c >= 0x80) {
      r = copy_utf8(&w, r, end);
    } else {
      *w++ = *r++;
    }
  }
  *w = '\0';
}

int opts_split_lines(char *buf, char ***out_rows) {
  size_t cap = 16;
  char **rows = xmalloc(cap * sizeof(*rows));
  int count = 0;
  char *start = buf;
  for (char *p = buf;; p++) {
    if (*p == '\n' || *p == '\0') {
      char end = *p;
      *p = '\0';
      opts_sanitize_row(start);
      if ((size_t)count == cap) {
        cap *= 2;
        rows = xrealloc(rows, cap * sizeof(*rows));
      }
      rows[count++] = start;
      if (end == '\0') break;
      start = p + 1;
    }
  }
  while (count > 0 && rows[count - 1][0] == '\0') count--;
  *out_rows = rows;
  return count;
}
