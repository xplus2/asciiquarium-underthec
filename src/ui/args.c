#include "args.h"
#include "cli_help.h"
#include "../opts.h"
#include "../version.h"

#include <stdlib.h>
#include <string.h>

void write_parts(FILE *stream, const char *const *parts, size_t count) {
  char buf[512] = {0};
  size_t pos = 0;
  for (size_t i = 0; i < count; i++) opts_append_bounded(buf, sizeof(buf), &pos, parts[i]);
  fwrite(buf, 1, pos, stream);
}

static int err_requires_arg(const char *prog, const char *opt) {
  write_parts(stderr, (const char *[]){prog, ": ", opt, " requires an argument\n"}, 4);
  return 2;
}

static int err_env_bad(const char *prog, const char *name, const char *errbuf) {
  write_parts(stderr, (const char *[]){prog, ": ", errbuf, " for ", name, "\n"}, 6);
  return 2;
}

static bool parse_teletext_mode(const char *val, enum tt_mode *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "t42") == 0) *out = TT_T42;
  else if (strcmp(val, "ts") == 0) *out = TT_TS;
  else {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid teletext format '", val, "', expected t42 or ts"}, 3);
    return false;
  }
  return true;
}

static bool parse_teletext_glyphs(const char *val, enum tt_glyphs *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "text") == 0) *out = TT_TEXT;
  else if (strcmp(val, "mosaic") == 0) *out = TT_MOSAIC;
  else {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid teletext mode '", val, "', expected text or mosaic"}, 3);
    return false;
  }
  return true;
}

static bool parse_ttl(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  if (!opts_parse_int_range(val, 1, 255, out)) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid ttl '", val, "', expected 1-255"}, 3);
    return false;
  }
  return true;
}

/* shared opt + env fallback table: (val, dest, errbuf, errbuf_len) -> ok */
typedef bool (*opt_parser_fn)(const char *val, void *out, char *errbuf, size_t errbuf_len);

static bool w_bool(const char *val, void *out, char *errbuf, size_t errbuf_len) {
  return opts_parse_bool(val, out, errbuf, errbuf_len);
}
static bool w_ttl(const char *val, void *out, char *errbuf, size_t errbuf_len) {
  return parse_ttl(val, out, errbuf, errbuf_len);
}

/* buffer-out parsers (val, buf, cap, errbuf, errbuf_len) wrapped behind opt_parser_fn */
struct buf_arg {
  char *buf;
  size_t cap;
  bool (*parse)(const char *val, char *out, size_t out_cap, char *errbuf, size_t errbuf_len);
};

static bool w_buf(const char *val, void *out, char *errbuf, size_t errbuf_len) {
  struct buf_arg *ba = out;
  return ba->parse(val, ba->buf, ba->cap, errbuf, errbuf_len);
}

struct cli_opt {
  const char *short_opt;
  const char *long_opt;
  opt_parser_fn parse;
  void *out;
  bool *given;
};

static bool apply_env(bool given, const char *env_name, const char *prog, opt_parser_fn parse, void *out, int *exit_code) {
  if (given) return true;
  const char *env_val = getenv(env_name);
  if (env_val == NULL) return true;
  char errbuf[128];
  if (!parse(env_val, out, errbuf, sizeof errbuf)) {
    *exit_code = err_env_bad(prog, env_name, errbuf);
    return false;
  }
  return true;
}

/* CLI opts by struct */
struct cfg_opt {
  const char *short_opt;
  const char *long_opt;
  const char *name;
};

static const struct cfg_opt cfg_opts[] = {
  {"-p", "--pace", "pace"},
  {"-u", "--uturn-chance", "uturn-chance"},
  {"-C", "--colors", "colors"},
  {"-f", "--fps", "fps"},
  {"-n", "--castle-name", "castle-name"},
  {"-P", "--message-position", "message-position"},
  {"-m", "--message", "message"},
  {NULL, "--time-color", "time-color"},
  {"-M", "--message-color", "message-color"},
  {"-a", "--aquatic-life", "aquatic-life"},
};

/* env never sets these in classic, set = from cmdline */
static const struct {
  const char *name;
  const char *shown;
} classic_conflicts[] = {
  {"aquatic-life", "-a/--aquatic-life"},
  {"fps", "-f/--fps"},
  {"uturn-chance", "-u/--uturn-chance"},
  {"message", "-m/--message"},
  {"message-color", "-M/--message-color"},
  {"message-position", "-P/--message-position"},
  {"time", "--time"},
  {"time-color", "--time-color"},
  {"castle-name", "-n/--castle-name"},
};

static bool cfg_error(const char *prog, const char *err, int *exit_code) {
  write_parts(stderr, (const char *[]){prog, ": ", err, "\n"}, 4);
  *exit_code = 2;
  return false;
}

static bool opt_hit(const char *a, const char *short_opt, const char *long_opt) {
  return (short_opt != NULL && strcmp(a, short_opt) == 0) || (long_opt != NULL && strcmp(a, long_opt) == 0);
}

bool args_parse(int argc, char **argv, struct cli_args *out, struct config *cfg, int *exit_code) {
  bool t_given = false;
  bool s_given = false;
  bool glyphs_given = false;
  bool ttl_given = false;
  bool teletext_caption_given = false;
  const char *teletext_arg = NULL;
  const char *glyphs_arg = NULL;
  char err[256];

  config_init(cfg);
  out->screensaver = false;
  out->transparent = false;
  out->mcast_arg = NULL;
  out->iface_arg = NULL;
  out->mcast_ttl = 1;
  { size_t p = 0; opts_append_bounded(out->teletext_caption, sizeof(out->teletext_caption) - 1, &p, TT_TITLE); out->teletext_caption[p] = '\0'; }

  struct buf_arg teletext_caption_arg = { out->teletext_caption, sizeof(out->teletext_caption), opts_parse_teletext_caption };

  struct cli_opt table[] = {
    {NULL, "--ttl", w_ttl, &out->mcast_ttl, &ttl_given},
    {NULL, "--teletext-caption", w_buf, &teletext_caption_arg, &teletext_caption_given},
  };

  int i = 1;
  while (i < argc) {
    const char *a = argv[i];
    bool matched = false;
    for (size_t k = 0; k < sizeof(cfg_opts) / sizeof(cfg_opts[0]); k++) {
      if (!opt_hit(a, cfg_opts[k].short_opt, cfg_opts[k].long_opt)) continue;
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      if (!config_set(cfg, cfg_opts[k].name, argv[i + 1], a, err, sizeof err)) return cfg_error(argv[0], err, exit_code);
      i += 2;
      matched = true;
      break;
    }
    if (matched) continue;
    for (size_t k = 0; k < sizeof(table) / sizeof(table[0]); k++) {
      if (!opt_hit(a, table[k].short_opt, table[k].long_opt)) continue;
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      char errbuf[128];
      if (!table[k].parse(argv[i + 1], table[k].out, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        *exit_code = 2;
        return false;
      }
      *table[k].given = true;
      i += 2;
      matched = true;
      break;
    }
    if (matched) continue;
    if (strcmp(a, "-c") == 0 || strcmp(a, "--classic") == 0) {
      const char *ver = "";
      i++;
      if (i < argc && (strcmp(argv[i], "1.0") == 0 || strcmp(argv[i], "1.1") == 0)) ver = argv[i++];
      if (!config_set(cfg, "classic", ver, a, err, sizeof err)) return cfg_error(argv[0], err, exit_code);
    } else if (strcmp(a, "--time") == 0) {
      const char *fmt = "";
      i++;
      if (i < argc && (strcmp(argv[i], "24") == 0 || strcmp(argv[i], "12") == 0)) fmt = argv[i++];
      if (!config_set(cfg, "time", fmt, a, err, sizeof err)) return cfg_error(argv[0], err, exit_code);
    } else if (strcmp(a, "-s") == 0 || strcmp(a, "--screensaver") == 0) {
      out->screensaver = true;
      s_given = true;
      i++;
    } else if (strcmp(a, "-t") == 0 || strcmp(a, "--transparent") == 0) {
      out->transparent = true;
      t_given = true;
      i++;
    } else if (strncmp(a, "--teletext=", 11) == 0) {
      teletext_arg = a + 11;
      i++;
    } else if (strcmp(a, "--teletext") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      teletext_arg = argv[i + 1];
      i += 2;
    } else if (strncmp(a, "--teletext-mode=", 16) == 0) {
      glyphs_arg = a + 16;
      glyphs_given = true;
      i++;
    } else if (strcmp(a, "--teletext-mode") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      glyphs_arg = argv[i + 1];
      glyphs_given = true;
      i += 2;
    } else if (strcmp(a, "--mcast") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      out->mcast_arg = argv[i + 1];
      i += 2;
    } else if (strcmp(a, "--iface") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      out->iface_arg = argv[i + 1];
      i += 2;
    } else if (strcmp(a, "--no-castle") == 0) {
      if (!config_set(cfg, "no-castle", "1", a, err, sizeof err)) return cfg_error(argv[0], err, exit_code);
      i++;
    } else if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
      print_help(argv[0]);
      *exit_code = 0;
      return false;
    } else if (strcmp(a, "-v") == 0 || strcmp(a, "--version") == 0) {
      write_parts(stdout, (const char *[]){
        TOOL_NAME, " v", TOOL_VERSION, "\n",
        "License GPLv2: GNU GPL version 2 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>\n",
        "This is free software; you are free to change and redistribute it.\n",
        "There is NO WARRANTY, to the extent permitted by law.\n"
      }, 7);
      *exit_code = 0;
      return false;
    } else {
      write_parts(stderr, (const char *[]){argv[0], ": unknown option '", a, "'\n"}, 4);
      print_help(argv[0]);
      *exit_code = 2;
      return false;
    }
  }

  if (!config_apply_env(cfg, err, sizeof err)) return cfg_error(argv[0], err, exit_code);
  if (!apply_env(t_given, "UNDERTHEC_TRANSPARENT", argv[0], w_bool, &out->transparent, exit_code)) return false;
  if (!apply_env(s_given, "UNDERTHEC_SCREENSAVER", argv[0], w_bool, &out->screensaver, exit_code)) return false;
  if (teletext_arg == NULL) teletext_arg = getenv("UNDERTHEC_TELETEXT");
  if (glyphs_arg == NULL) glyphs_arg = getenv("UNDERTHEC_TELETEXT_MODE");
  if (out->mcast_arg == NULL) out->mcast_arg = getenv("UNDERTHEC_MCAST");
  if (out->iface_arg == NULL) out->iface_arg = getenv("UNDERTHEC_MCAST_IFACE");
  if (!apply_env(ttl_given, "UNDERTHEC_MCAST_TTL", argv[0], w_ttl, &out->mcast_ttl, exit_code)) return false;
  if (!apply_env(teletext_caption_given, "UNDERTHEC_TELETEXT_CAPTION", argv[0], w_buf, &teletext_caption_arg, exit_code)) return false;

  if (cfg->classic_ver != 0) {
    for (size_t k = 0; k < sizeof(classic_conflicts) / sizeof(classic_conflicts[0]); k++) {
      if (!config_is_set(cfg, classic_conflicts[k].name)) continue;
      write_parts(stderr, (const char *[]){argv[0], ": classic mode and ", classic_conflicts[k].shown, " are mutually exclusive\n"}, 4);
      *exit_code = 2;
      return false;
    }
  }
  if (cfg->time_mode != 0 && config_is_set(cfg, "message-position") && cfg->message_position == MSG_POS_MIDDLE) {
    write_parts(stderr, (const char *[]){argv[0], ": --time and -P/--message-position middle are mutually exclusive\n"}, 2);
    *exit_code = 2;
    return false;
  }
  if (!config_check(cfg, err, sizeof err)) return cfg_error(argv[0], err, exit_code);

  out->teletext_requested = (teletext_arg != NULL);
  out->mcast_requested = (out->mcast_arg != NULL);
  out->tt_mode = TT_TS;
  out->tt_glyphs = TT_TEXT;
  if (glyphs_given && !out->teletext_requested && !out->mcast_requested) {
    write_parts(stderr, (const char *[]){argv[0], ": --teletext-mode requires --teletext or --mcast\n"}, 2);
    *exit_code = 2;
    return false;
  }
  if (out->teletext_requested || out->mcast_requested) {
    char errbuf[128];
    if (out->teletext_requested && !parse_teletext_mode(teletext_arg, &out->tt_mode, errbuf, sizeof errbuf)) {
      write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, "\n"}, 4);
      *exit_code = 2;
      return false;
    }
    if (glyphs_arg != NULL && !parse_teletext_glyphs(glyphs_arg, &out->tt_glyphs, errbuf, sizeof errbuf)) {
      write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, "\n"}, 4);
      *exit_code = 2;
      return false;
    }
    if (out->mcast_requested && out->tt_mode == TT_T42) {
      write_parts(stderr, (const char *[]){argv[0], ": --mcast requires the ts format\n"}, 2);
      *exit_code = 2;
      return false;
    }
  }
  *exit_code = 0;
  return true;
}
