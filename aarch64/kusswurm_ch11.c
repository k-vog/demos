#include <stdint.h>
#include <stdio.h>

// example 1
int32_t i32_add_sub(int32_t a, int32_t b, int32_t c);
int64_t i64_add_sub(int64_t a, int64_t b, int64_t c);

// example 2
int32_t i32_mul(int32_t a, int32_t b);
int64_t i32_mul_safe(int32_t a, int32_t b);

#define PRINT_LOG(FMT, CODE) printf(#CODE ": " FMT "\n", CODE)

int main(int argc, const char* argv[])
{
    // example 1
    PRINT_LOG("%d", i32_add_sub(5, 3, 1));
    PRINT_LOG("%lld", i64_add_sub(5, 3, 1));

    // example 2
    PRINT_LOG("%d", i32_mul(5, 6));
    PRINT_LOG("%d", i32_mul(2000000000, 4));
    PRINT_LOG("%lld", i32_mul_safe(5, 6));
    PRINT_LOG("%lld", i32_mul_safe(2000000000, 4));
}

