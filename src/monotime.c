#define _DEFAULT_SOURCE

#include "monotime.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

double monotime_now(void) {
#if defined(_WIN32)
  static LARGE_INTEGER freq;
  if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
  LARGE_INTEGER t;
  QueryPerformanceCounter(&t);
  return (double)t.QuadPart / (double)freq.QuadPart;
#elif defined(__DJGPP__)
  return (double)uclock() / (double)UCLOCKS_PER_SEC;
#elif defined(__APPLE__)
  return (double)clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1e9;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#endif
}
