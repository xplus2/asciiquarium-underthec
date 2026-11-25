#ifndef UNDERTHEC_UTF8_H
#define UNDERTHEC_UTF8_H

#include <stddef.h>

int utf8_seq_len(unsigned char lead);
/* lenient, stops at broken continuation */
unsigned utf8_decode(const char *s, int seq_len);
/* bytes used or -1: bad lead/continuation, overlong, surrogate, > U+10FFFF */
int utf8_decode_strict(const char *s, size_t len, unsigned *cp_out);
/* 1 on broken continuation */
int utf8_valid_len(const char *s);
/* 0, 1 or 2 cols */
int utf8_char_width(const char *s, int seq_len);
int utf8_col_width(const char *s);
/* byte offset of char covering col */
int utf8_byte_offset(const char *s, int col);

#endif
