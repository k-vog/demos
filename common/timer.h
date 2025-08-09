#ifndef _DCOMMON_TIMER_H_
#define _DCOMMON_TIMER_H_

#include <stdint.h>

#ifdef _WIN32
# include <windows.h>
#endif

#ifdef __APPLE__
# include <mach/mach_time.h>
#endif

static inline uint64_t timer_timestamp(void)
{
#ifdef _WIN32
  LARGE_INTEGER t;
  QueryPerformanceCounter(&t);
  return (uint64_t)t.QuadPart;
#endif
#ifdef __APPLE__
  return mach_absolute_time();
#endif
}

static inline double timer_elapsed(uint64_t t0, uint64_t t1)
{
#ifdef _WIN32
  static double ns_per_tick = 0.0f;
  if (ns_per_tick == 0.0f) {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    ns_per_tick = 1e9f / (double)f.QuadPart;
  }
  return ((double)t1 - t0) * ns_per_tick;
#endif
#ifdef __APPLE__
  static mach_timebase_info_data_t tb;
  if (tb.denom == 0) {
    mach_timebase_info(&tb);
  }
  return ((double)(t1 - t0)) * tb.numer / tb.denom;
#endif
}

#endif // _DCOMMON_TIMER_H_
