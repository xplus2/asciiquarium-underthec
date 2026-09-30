#define _DEFAULT_SOURCE

#include "../../app.h"
#include "../../config.h"
#include "../terminal/term.h"

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static const struct {
  const char *env;
  const char *opt;
} CONFIG_ENV_VARS[] = {
    {"UNDERTHEC_CLASSIC", "classic"},
    {"UNDERTHEC_AQUATIC_LIFE", "aquatic-life"},
    {"UNDERTHEC_MESSAGE", "message"},
    {"UNDERTHEC_MESSAGE_COLOR", "message-color"},
    {"UNDERTHEC_MESSAGE_POSITION", "message-position"},
    {"UNDERTHEC_CASTLE_NAME", "castle-name"},
    {"UNDERTHEC_NO_CASTLE", "no-castle"},
    {"UNDERTHEC_PACE", "pace"},
    {"UNDERTHEC_UTURN_CHANCE", "uturn-chance"},
    {"UNDERTHEC_FPS", "fps"},
    {"UNDERTHEC_COLORS", "colors"},
};

static bool build_config(struct config *cfg) {
  config_init(cfg);
  char err[256];
  for (size_t i = 0; i < sizeof(CONFIG_ENV_VARS) / sizeof(CONFIG_ENV_VARS[0]); i++) {
    const char *value = getenv(CONFIG_ENV_VARS[i].env);
    if (value == NULL) continue;
    if (!config_set(cfg, CONFIG_ENV_VARS[i].opt, value, CONFIG_ENV_VARS[i].env, err, sizeof err)) {
      fprintf(stderr, "underthec vlock plugin: %s\n", err);
      config_free(cfg);
      return false;
    }
  }
  if (!config_check(cfg, err, sizeof err)) {
    fprintf(stderr, "underthec vlock plugin: %s\n", err);
    config_free(cfg);
    return false;
  }
  return true;
}

static double now_seconds(void) {
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static volatile sig_atomic_t g_should_quit = 0;

static void on_term(int sig) {
  (void)sig;
  g_should_quit = 1;
}

static void child_run(void) {
  if (term_init() != 0) _exit(1);
  struct sigaction sa;
  memset(&sa, 0, sizeof sa);
  sa.sa_handler = on_term;
  sigaction(SIGTERM, &sa, NULL);

  struct config cfg;
  if (!build_config(&cfg)) {
    term_shutdown();
    _exit(1);
  }

  struct app app;
  config_start(&cfg, &app, now_seconds());
  config_free(&cfg);

  double deadline = app.last;
  while (!g_should_quit) {
    int w;
    int h;
    term_size(&w, &h);
    app_resize(&app, w, h);
    double frame_period = 1.0 / (double)app.fps;
    deadline += frame_period;
    double wait = deadline - now_seconds();
    if (wait < -frame_period) deadline = now_seconds();
    if (wait > 0.0) {
      struct timespec ts;
      ts.tv_sec = (time_t)wait;
      ts.tv_nsec = (long)((wait - (double)ts.tv_sec) * 1e9);
      nanosleep(&ts, NULL);
    }
    app_frame(&app, now_seconds());
    term_present(&app.canvas);
  }
  app_free(&app);
  term_shutdown();
  _exit(0);
}

static void kill_child(void **ctx_ptr) {
  pid_t pid = (pid_t)(intptr_t)*ctx_ptr;
  if (pid > 0) {
    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);
  }
  *ctx_ptr = NULL;
}

bool vlock_save(void **ctx_ptr) {
  pid_t pid = fork();
  if (pid < 0) return false;
  if (pid == 0) {
    child_run();
    _exit(0);
  }
  *ctx_ptr = (void *)(intptr_t)pid;
  return true;
}

bool vlock_save_abort(void **ctx_ptr) {
  kill_child(ctx_ptr);
  return true;
}

bool vlock_end(void **ctx_ptr) {
  kill_child(ctx_ptr);
  return true;
}
