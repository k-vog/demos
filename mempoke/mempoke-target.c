// ================================================================================================
// Example showing how to read/write process memory remotely.
//
// License:
//     Copyright (c) 2026 Hunter Kvalevog
//
//     Permission to use, copy, modify, and/or distribute this software for any
//     purpose with or without fee is hereby granted.
//
//     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//     WITH REGARD TO THIS SOFTWARE.
// ================================================================================================

#ifdef _WIN32
#   include <windows.h>
#endif

#ifdef __linux__
#   include <unistd.h>
#endif

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

int tgtnum = 1;

static inline uintptr_t get_process_base_address(void)
{
    uintptr_t base = 0;
#ifdef _WIN32
    base = (uintptr_t)GetModuleHandleW(0);
#endif
#ifdef __linux__
    FILE *maps = fopen("/proc/self/maps", "r");
    fscanf(maps, "%" SCNxPTR "-", &base);
    fclose(maps);
#endif
    assert(base);
    return base;
}

static inline void sleep_ms(unsigned ms)
{
#ifdef _WIN32
    Sleep(ms);
#endif
#ifdef __linux__
    usleep(ms * 1000);
#endif
}

int main(int argc, const char **argv)
{
    (void)argc;
    (void)argv;

    printf("offset: 0x%" PRIxPTR  "\n", (uintptr_t)&tgtnum - get_process_base_address());
    sleep_ms(3000);

    while (1) {
        sleep_ms(1000);
        printf("tgtnum: %d\n", tgtnum);
    }
}

