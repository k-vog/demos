#ifndef _DCOMMON_BASE_H_
#define _DCOMMON_BASE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Core types
// -----------------------------------------------------------------------------

using u8  = uint8_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i32 = int32_t;

using f32 = float;
using f64 = double;

using usize = size_t;

// -----------------------------------------------------------------------------
// Memory allocation helpers
// -----------------------------------------------------------------------------

template <typename T>
static inline T* MemAlloc(usize count = 1)
{
  T* mem = (T*)malloc(sizeof(T) * count);
  return mem;
}

template <typename T>
static inline T* MemAllocZ(usize count = 1)
{
  T* mem = (T*)calloc(count, sizeof(T));
  return mem;
}

template <typename T>
static inline void MemFree(T* mem)
{
  free(mem);
}

#endif // _DECOMMON_BASE_H_