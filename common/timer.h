#ifndef _DCOMMON_TIMER_H_
#define _DCOMMON_TIMER_H_

#include <stdint.h>

#ifdef __APPLE__
# include <mach/mach_time.h>
#endif

static inline uint64_t timer_timestamp(void)
{
#ifdef __APPLE__
  return mach_absolute_time();
#endif
}

static inline double timer_elapsed(uint64_t t0, uint64_t t1)
{
#ifdef __APPLE__
  static mach_timebase_info_data_t tb;
  if (tb.denom == 0) {
    mach_timebase_info(&tb);
  }
  return ((double)(t1 - t0)) * tb.numer / tb.denom;
#endif
}

#endif // _DCOMMON_TIMER_H_
