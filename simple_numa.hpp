#pragma once
#include <sys/syscall.h>
#include <unistd.h>

#if defined(__linux__)
#    define MPOL_DEFAULT   0
#    define MPOL_BIND      2
#    define MPOL_MF_STRICT (1 << 0)
#    define MPOL_MF_MOVE   (1 << 1)
#if !defined(__NR_mbind) && defined(__x86_64__)
#    define __NR_mbind 237
#endif

static long mbind(void* start,
                  unsigned long len,
                  int mode,
                  const unsigned long* nmask,
                  unsigned long maxnode,
                  unsigned flags) {
    return syscall(__NR_mbind, (long)start, len, mode, (long)nmask, maxnode, flags);
}
#endif

#if defined(__linux__)
bool mbind_move(void* data, size_t size, int targetNode) {
    // int realNode = ov::get_org_numa_id(targetNode);
    int realNode = targetNode;
    auto pagesize = getpagesize();
    auto page_count = (size + pagesize - 1) / pagesize;
    char* pages = reinterpret_cast<char*>((((uintptr_t)data) & ~((uintptr_t)(pagesize - 1))));
    unsigned long mask = 0;
    unsigned flags = 0;
    if (realNode < 0) {
        // restore default policy
        mask = -1;
        flags = 0;
    } else {
        mask = 1ul << realNode;
        flags = MPOL_MF_MOVE | MPOL_MF_STRICT;
    }

    auto rc = mbind(pages, page_count * pagesize, MPOL_BIND, &mask, sizeof(mask) * 8, flags);
    if (rc < 0) {
        DEBUG_LOG("mbind failed: ", strerror(errno));
        return false;
    }
    return true;
}
#else
bool mbind_move(void* data, size_t size, int targetNode) {
    return false;
}
#endif
