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

#define _GNU_SOURCE

#ifdef _WIN32
#   include <windows.h>
#   include <psapi.h>
#   include <tlhelp32.h>
#endif

#ifdef __linux__
#   include <dirent.h>
#   include <sys/uio.h>
#endif

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ProcessHandle ProcessHandle;
struct ProcessHandle
{
    int       pid;
    uintptr_t base;
#ifdef _WIN32
    HANDLE    handle;
#endif
};

static inline void open_target_process(ProcessHandle *p)
{
#ifdef _WIN32
    // ref: https://docs.microsoft.com/en-us/windows/win32/toolhelp/taking-a-snapshot-and-viewing-processes
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    assert(snap != INVALID_HANDLE_VALUE);

    PROCESSENTRY32 pe = { .dwSize = sizeof(PROCESSENTRY32) };
    Process32First(snap, &pe);

    do {
        if (!strcmp(pe.szExeFile, "mempoke-target.exe")) {
            p->pid    = pe.th32ProcessID;
            p->handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, p->pid);
            assert(p->handle && "failed to open process");

            // First module is always the exe itself
            HMODULE exe    = 0;
            DWORD   needed = 0;
            if (!EnumProcessModules(p->handle, &exe, sizeof(exe), &needed)) {
                assert(0 && "EnumProcessModules failed");
            }
            p->base = (uintptr_t)exe;

            return;
        }
    } while (Process32Next(snap, &pe));
#endif
#ifdef __linux__
    DIR *proc = opendir("/proc");

    struct dirent *d;
    while ((d = readdir(proc))) {
        pid_t pid = (pid_t)atoi(d->d_name);
        if (pid <= 0)
            continue;

        char comm[256];
        snprintf(comm, sizeof(comm), "/proc/%d/comm", pid);

        FILE *f = fopen(comm, "r");
        if (!f)
            continue;

        if (fgets(comm, sizeof(comm), f) && !strncmp(comm, "mempoke-target", 14)) {
            p->pid = pid;

            char maps_path[64];
            snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", p->pid);

            FILE *maps = fopen(maps_path, "r");
            fscanf(maps, "%" SCNxPTR "-", &p->base);
            fclose(maps);
            fclose(f);
            return;
        }

        fclose(f);
    }
#endif
    assert(0 && "couldn't find process");
}

int main(int argc, const char **argv)
{
    const uintptr_t offset = (argc > 1) ? strtoull(argv[1], 0, 0) : 0;
    if (!offset) {
        printf("Usage: %s <offset>\n    example: %s 0x1234\n", argv[0], argv[0]);
        return 1;
    }

    ProcessHandle p;
    open_target_process(&p);

    printf("target process: pid=%d base=0x%" PRIxPTR  "\n", p.pid, p.base);

    int       num = 0;
    uintptr_t tgt = p.base + offset;

#ifdef _WIN32
    if (!ReadProcessMemory(p.handle, (LPCVOID)tgt, &num, sizeof(num), 0)) {
        assert(0 && "ReadProcessMemory failed");
    }
#endif
#ifdef __linux__
    struct iovec iov_rsrc = { .iov_base = (void *)tgt, .iov_len = sizeof(num) };
    struct iovec iov_rdst = { .iov_base = &num,        .iov_len = sizeof(num) };
    if (process_vm_readv(p.pid, &iov_rdst, 1, &iov_rsrc, 1, 0) != sizeof(num)) {
        assert(0 && "process_vm_readv failed");
    }
#endif

    printf("tgtnum = %d\n", num);

    num += 1;

#ifdef _WIN32
    if (!WriteProcessMemory(p.handle, (LPVOID)tgt, &num, sizeof(num), 0)) {
        assert(0 && "WriteProcessMemory failed");
    }
#endif
#ifdef __linux__
    struct iovec iov_wsrc = { .iov_base = &num,        .iov_len = sizeof(num) };
    struct iovec iov_wdst = { .iov_base = (void *)tgt, .iov_len = sizeof(num) };
    if (process_vm_writev(p.pid, &iov_wsrc, 1, &iov_wdst, 1, 0) != sizeof(num)) {
        assert(0 && "process_vm_writev failed");
    }
#endif
}

