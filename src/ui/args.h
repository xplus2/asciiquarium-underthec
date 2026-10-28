#ifndef UNDERTHEC_UI_ARGS_H
#define UNDERTHEC_UI_ARGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "../config.h"
#include "../target/teletext/teletext.h"

/* terminal-only, rest in struct */
struct cli_args {
  bool screensaver;
  bool transparent;

  bool teletext_requested;
  bool mcast_requested;
  enum tt_mode tt_mode;
  enum tt_glyphs tt_glyphs;
  const char *mcast_arg;
  const char *iface_arg;
  int mcast_ttl;
  char teletext_caption[TT_HDR_LEN + 1];
};

/* cfg inited here, caller frees. false: *exit_code 0 = h/v, 2 = usage error, caller ret as-is */
bool args_parse(int argc, char **argv, struct cli_args *out, struct config *cfg, int *exit_code);

void write_parts(FILE *stream, const char *const *parts, size_t count);

#endif
