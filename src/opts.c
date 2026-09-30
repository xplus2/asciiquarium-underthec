#include "opts.h"
#include "color.h"
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

static void append_char_bounded(char *dst, size_t dst_cap, size_t *pos, char c) {
  if (*pos < dst_cap) dst[(*pos)++] = c;
}

void opts_append_char_bounded(char *dst, size_t dst_cap, size_t *pos, char c) {
  append_char_bounded(dst, dst_cap, pos, c);
}

void opts_append_bounded_w(char *dst, size_t dst_cap, size_t *pos, const char *src, int width) {
  size_t start = *pos;
  opts_append_bounded(dst, dst_cap, pos, src);
  int written = (int)(*pos - start);
  for (int i = written; i < width; i++) append_char_bounded(dst, dst_cap, pos, ' ');
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
  for (int i = total; i < width; i++) append_char_bounded(dst, dst_cap, pos, ' ');
  if (neg) append_char_bounded(dst, dst_cap, pos, '-');
  while (n > 0) append_char_bounded(dst, dst_cap, pos, tmp[--n]);
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
  for (int i = (int)blen; i < width; i++) append_char_bounded(dst, dst_cap, pos, ' ');
  opts_append_bounded(dst, dst_cap, pos, body);
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
  if (!opts_parse_int_range(val, 0, 999, out)) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid fish count '", val, "', expected 0-999 or auto"}, 3);
    return false;
  }
  return true;
}

bool opts_parse_aquatic_life(const char *definition, struct aquatic_life *out, bool allow_fish, bool *fish_set, char *errbuf, size_t errbuf_len) {
  scene_aquatic_fill(out, false);
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
      if (!allow_fish) {
        opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"fish must be set via UNDERTHEC_FISH, not here"}, 1);
        ok = false;
      } else if (!opts_parse_fish_count(token + 5, &out->fish_count, errbuf, errbuf_len)) {
        ok = false;
      } else {
        *fish_set = true;
      }
    } else if (!scene_aquatic_set_flag(out, token)) {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"unknown aquatic-life entry '", token, "'"}, 3);
      ok = false;
    }
    token = buf + i + 1;
  }
  free(buf);
  return ok;
}

bool opts_parse_classic(const char *val, int *out_ver, char *errbuf, size_t errbuf_len) {
  if (val[0] == '\0' || strcmp(val, "1.0") == 0) {
    *out_ver = 1;
    return true;
  }
  if (strcmp(val, "1.1") == 0) {
    *out_ver = 2;
    return true;
  }
  opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid value '", val, "', expected 1.0 or 1.1"}, 3);
  return false;
}

bool opts_parse_message_position(const char *val, enum message_position *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "middle") == 0) *out = MSG_POS_MIDDLE;
  else if (strcmp(val, "center") == 0) *out = MSG_POS_CENTER;
  else if (strcmp(val, "marquee") == 0) *out = MSG_POS_MARQUEE;
  else if (strcmp(val, "swim") == 0) *out = MSG_POS_SWIM;
  else if (strcmp(val, "event") == 0) *out = MSG_POS_EVENT;
  else {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid message position '", val, "'"}, 3);
    return false;
  }
  return true;
}

bool opts_parse_uturn_chance(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  if (!opts_parse_int_range(val, 0, 999, out)) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid uturn chance '", val, "', expected 0-999"}, 3);
    return false;
  }
  return true;
}

bool opts_parse_fps(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  if (!opts_parse_int_range(val, 1, 240, out)) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid fps '", val, "', expected 1-240"}, 3);
    return false;
  }
  return true;
}

bool opts_parse_colors(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  const char *dash = strchr(val, '-');
  if (dash != NULL) {
    if (dash == val + 1 && val[0] == '8') {
      if (strcasecmp(dash + 1, "bold") == 0) {
        *out = 108;
        return true;
      }
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"colors accent '", val, "' only valid for 1, 2 or 8-bold"}, 3);
      return false;
    }
    if ((dash != val + 1) || (val[0] != '1' && val[0] != '2')) {
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

bool opts_parse_pace(const char *s, double *out, char *errbuf, size_t errbuf_len) {
  size_t dot_count = 0;
  for (const char *p = s; *p != '\0'; p++) {
    if (*p == '.') {
      dot_count++;
      if (dot_count > 1) {
        opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid pace '", s, "'"}, 3);
        return false;
      }
    } else if (*p < '0' || *p > '9') {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid pace '", s, "'"}, 3);
      return false;
    }
  }
  const char *dot = strchr(s, '.');
  if (dot != NULL && strlen(dot + 1) > 2) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"pace '", s, "' has more than 2 decimal digits"}, 3);
    return false;
  }
  char *endptr = NULL;
  double val = strtod(s, &endptr);
  if (s[0] == '\0' || *endptr != '\0') {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid pace '", s, "'"}, 3);
    return false;
  }
  if (val < 0.01 - 1e-9 || val > 10.0 + 1e-9) {
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

static int castle_name_utf8_decode(const char *s, size_t len, unsigned *cp_out) {
  unsigned char b0 = (unsigned char)s[0];
  if (b0 < 0x80) {
    *cp_out = b0;
    return 1;
  }
  int n;
  unsigned cp;
  if ((b0 & 0xE0) == 0xC0) {
    if (b0 < 0xC2) return -1;
    n = 2;
    cp = b0 & 0x1F;
  } else if ((b0 & 0xF0) == 0xE0) {
    n = 3;
    cp = b0 & 0x0F;
  } else if ((b0 & 0xF8) == 0xF0) {
    if (b0 > 0xF4) return -1;
    n = 4;
    cp = b0 & 0x07;
  } else {
    return -1;
  }
  if (len < (size_t)n) return -1;
  for (int k = 1; k < n; k++) {
    unsigned char c = (unsigned char)s[k];
    if ((c & 0xC0) != 0x80) return -1;
    cp = (cp << 6) | (unsigned)(c & 0x3F);
  }
  if (n == 3 && (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF))) return -1;
  if (n == 4 && (cp < 0x10000 || cp > 0x10FFFF)) return -1;
  *cp_out = cp;
  return n;
}

bool opts_parse_castle_name(const char *val, char *out, size_t out_cap, char *errbuf, size_t errbuf_len) {
  size_t len = strlen(val);
  for (size_t i = 0; i < len; ) {
    unsigned cp;
    int n = castle_name_utf8_decode(val + i, len - i, &cp);
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
  if (entity_utf8_display_width(val) > CASTLE_NAME_LEN) {
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

int opts_split_lines(char *buf, char ***out_rows) {
  size_t cap = 16;
  char **rows = xmalloc(cap * sizeof(*rows));
  int count = 0;
  char *start = buf;
  for (char *p = buf;; p++) {
    if (*p == '\n' || *p == '\0') {
      char end = *p;
      *p = '\0';
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
