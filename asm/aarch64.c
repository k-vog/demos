#include <stdint.h>
#include <stdio.h>

// Add two integers
extern int addi32(int32_t i1, int32_t i2);

// Subtract two integers
extern int subi32(int32_t i1, int32_t i2);

// Add two floats
extern float addf32(float f1, float f2);

// Subtract two floats
extern float subf32(float f1, float f2);

// Add two vectors (scalar, vector)
extern void addf32v4_s(float* dst, const float* f1, const float* f32);
extern void addf32v4_v(float* dst, const float* f1, const float* f32);

int main(int argc, const char* argv[])
{
  // Scalar operations
#define LOG_CALL(fmt, expr) \
  printf("%s: " fmt "\n", #expr, expr)

  LOG_CALL("%d", addi32(1, 2));
  LOG_CALL("%d", addi32(0, -10));

  LOG_CALL("%d", subi32(3, 2));
  LOG_CALL("%d", subi32(10, 1000));

  LOG_CALL("%f", addf32(1.0f, 3.0f));

  LOG_CALL("%f", subf32(1.0f, 3.0f));

#undef LOG_CALL

  // Vector operations
  float f32v4_1[4];
  float f32v4_2[4];
  float f32v4_3[4];
#define LOG_CALL(fmt, fn, x1, y1, z1, w1, x2, y2, z2, w2)               \
  f32v4_1[0] = x1; f32v4_1[1] = y1; f32v4_1[2] = z1; f32v4_1[3] = w1;   \
  f32v4_2[0] = x2; f32v4_2[1] = y2; f32v4_2[2] = z2; f32v4_2[3] = w2;   \
  f32v4_3[0] =  0; f32v4_3[1] =  0; f32v4_3[2] =  0; f32v4_3[3] =  0;   \
  fn(f32v4_3, f32v4_1, f32v4_2);                                        \
  printf("%s([" fmt ", " fmt ", " fmt ", " fmt "], "                    \
            "[" fmt ", " fmt ", " fmt ", " fmt "]): "                   \
            "[" fmt ", " fmt ", " fmt ", " fmt "]\n",                   \
    #fn, x1, y1, z1, w1, x2, y2, z2, w2,                                \
    f32v4_3[0], f32v4_3[1], f32v4_3[2], f32v4_3[3])

  LOG_CALL("%.2f", addf32v4_s,
    1.0f, 2.0f, 3.0f, 4.0f,
    5.0f, 6.0f, 7.0f, 8.0f
  );
  LOG_CALL("%.2f", addf32v4_v,
    1.0f, 2.0f, 3.0f, 4.0f,
    5.0f, 6.0f, 7.0f, 8.0f
  );

#undef LOG_CALL
}
