#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
// Core type helpers
// -----------------------------------------------------------------------------

#define ASSERT assert

// Note: Only works on integer types
template <typename T>
static inline T Align(u32 x, u32 boundary)
{
  return ((x + boundary - 1) / boundary) * boundary;
};

template <typename T>
static inline T Min(T x1, T x2)
{
  return (x1 < x2) ? x1 : x2;
}

template <typename T>
static inline T Max(T x1, T x2)
{
  return (x1 > x2) ? x1 : x2;
}

template <typename T>
static inline T Clamp(T x, T x_min, T x_max)
{
  return Min(Max(x, x_min), x_max);
}

template <typename T>
static inline u8 ClampByte(T x)
{
  return (u8)Clamp<T>(x, 0x00, 0xFF);
}

template <typename T>
static inline bool InRange(T x, T x_min, T x_max)
{
  return x >= x_min && x <= x_max;
}

template <typename T>
static inline usize BitSize()
{
  return sizeof(T) * 8;
}

template <typename T>
static inline T RotateLeft(T x, usize n)
{
  return (x << n) | (x >> (BitSize<T>() - n)) & ~(-1 >> n);
}

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

// -----------------------------------------------------------------------------
// Span
// -----------------------------------------------------------------------------

template <typename T>
struct Span
{
  T*    buf;
  usize len;

  Span()
    : buf(buf), len(len)
  {
  }

  Span(T* buf, usize len)
    : buf(buf), len(len)
  {
  }

  usize LengthInBytes()
  {
    return len * sizeof(T);
  }
};

template <typename T>
static inline Span<T> HeapSpan(usize count)
{
  Span<T> result = { };
  result.len = count;
  result.buf = MemAlloc<T>(count);
  return result;
}

template <typename T>
static inline Span<T> HeapSpanZ(usize count)
{
  Span<T> result = { };
  result.len = count;
  result.buf = MemAllocZ<T>(count);
  return result;
}

