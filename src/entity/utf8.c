#include "priv.h"

int utf8_seq_len(unsigned char lead) {
  if ((lead & 0x80) == 0x00) return 1;
  if ((lead & 0xE0) == 0xC0) return 2;
  if ((lead & 0xF0) == 0xE0) return 3;
  if ((lead & 0xF8) == 0xF0) return 4;
  return 1;
}

unsigned utf8_decode(const char *s, int seq_len) {
  unsigned char c0 = (unsigned char)s[0];
  if (seq_len == 1) return c0;
  unsigned mask;
  if (seq_len == 2) mask = 0x1F;
  else if (seq_len == 3) mask = 0x0F;
  else mask = 0x07;
  unsigned cp = c0 & mask;
  for (int k = 1; k < seq_len; k++) {
    if (((unsigned char)s[k] & 0xC0) != 0x80) break;
    cp = (cp << 6) | (unsigned)((unsigned char)s[k] & 0x3F);
  }
  return cp;
}

int utf8_valid_len(const char *s) {
  int n = utf8_seq_len((unsigned char)s[0]);
  for (int k = 1; k < n; k++)
    if (((unsigned char)s[k] & 0xC0) != 0x80) return 1;
  return n;
}

/* most common cases: cjk, hangul, emoji blocks */
struct wide_range { unsigned lo; unsigned hi; };
static const struct wide_range wide_ranges[] = {
  {0x1100, 0x115F}, {0x231A, 0x231B}, {0x2329, 0x232A}, {0x23E9, 0x23EC},
  {0x23F0, 0x23F0}, {0x23F3, 0x23F3}, {0x25FD, 0x25FE}, {0x2614, 0x2615},
  {0x2648, 0x2653}, {0x267F, 0x267F}, {0x2693, 0x2693}, {0x26A1, 0x26A1},
  {0x26AA, 0x26AB}, {0x26BD, 0x26BE}, {0x26C4, 0x26C5}, {0x26CE, 0x26CE},
  {0x26D4, 0x26D4}, {0x26EA, 0x26EA}, {0x26F2, 0x26F3}, {0x26F5, 0x26F5},
  {0x26FA, 0x26FA}, {0x26FD, 0x26FD}, {0x2705, 0x2705}, {0x270A, 0x270B},
  {0x2728, 0x2728}, {0x274C, 0x274C}, {0x274E, 0x274E}, {0x2753, 0x2755},
  {0x2757, 0x2757}, {0x2795, 0x2797}, {0x27B0, 0x27B0}, {0x27BF, 0x27BF},
  {0x2B1B, 0x2B1C}, {0x2B50, 0x2B50}, {0x2B55, 0x2B55}, {0x2E80, 0x303E},
  {0x3041, 0x33FF}, {0x3400, 0x4DBF}, {0x4E00, 0x9FFF}, {0xA000, 0xA4CF},
  {0xAC00, 0xD7A3}, {0xF900, 0xFAFF}, {0xFE30, 0xFE4F}, {0xFF00, 0xFF60},
  {0xFFE0, 0xFFE6}, {0x1F1E6, 0x1F1FF}, {0x1F300, 0x1FAFF},{0x20000, 0x3FFFD},
};

/* combining marks, joiners, variation selectors */
static const struct wide_range zero_ranges[] = {
  {0x0300, 0x036F}, {0x0483, 0x0489}, {0x0591, 0x05BD}, {0x0610, 0x061A},
  {0x064B, 0x065F}, {0x0E31, 0x0E31}, {0x0E34, 0x0E3A}, {0x0E47, 0x0E4E},
  {0x1AB0, 0x1AFF}, {0x1DC0, 0x1DFF}, {0x200B, 0x200F}, {0x2028, 0x202E},
  {0x2060, 0x2064}, {0x20D0, 0x20FF}, {0xFE00, 0xFE0F}, {0xFE20, 0xFE2F},
  {0xFEFF, 0xFEFF}, {0xE0100, 0xE01EF},
};

static bool codepoint_in(const struct wide_range *tab, int n, unsigned cp) {
  int lo = 0;
  int hi = n - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    if (cp < tab[mid].lo) hi = mid - 1;
    else if (cp > tab[mid].hi) lo = mid + 1;
    else return true;
  }
  return false;
}

static bool codepoint_is_wide(unsigned cp) {
  return codepoint_in(wide_ranges, (int)(sizeof(wide_ranges) / sizeof(wide_ranges[0])), cp);
}

static bool codepoint_is_zero(unsigned cp) {
  return codepoint_in(zero_ranges, (int)(sizeof(zero_ranges) / sizeof(zero_ranges[0])), cp);
}

int utf8_char_width(const char *s, int seq_len) {
  if (seq_len == 1) return 1;
  unsigned cp = utf8_decode(s, seq_len);
  if (codepoint_is_zero(cp)) return 0;
  return codepoint_is_wide(cp) ? 2 : 1;
}

int utf8_col_width(const char *s) {
  int cols = 0;
  for (int i = 0; s[i] != '\0'; ) {
    int seq_len = utf8_valid_len(s + i);
    cols += utf8_char_width(s + i, seq_len);
    i += seq_len;
  }
  return cols;
}

int utf8_byte_offset(const char *s, int col) {
  int i = 0;
  int c = 0;
  while (s[i] != '\0') {
    int seq_len = utf8_valid_len(s + i);
    int width = utf8_char_width(s + i, seq_len);
    if (col < c + width) return i;
    c += width;
    i += seq_len;
  }
  return i;
}

int entity_utf8_display_width(const char *s) {
  return utf8_col_width(s);
}

int entity_utf8_byte_offset(const char *s, int col) {
  return utf8_byte_offset(s, col);
}
