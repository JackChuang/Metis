#ifndef BENCH_H
#define BENCH_H

#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <algorithm>
#include "platform.hh"

#define JOS_PAGESIZE    4096
enum { debug_print = 0 };

template <typename T, typename M>
inline T round_down(T n, M b) {
    uintptr_t r = uintptr_t(n);
    return (T)(r - r % b);
}

template <typename T, typename M>
inline T round_up(T n, M b) {
    uintptr_t r = uintptr_t(n);
    return round_down(r + b - 1, b);
}

template <typename T>
inline T *safe_malloc(int n = 1) {
    void *x = malloc(sizeof(T) * n);
    assert(n ==0 || x);
    return (T *)x;
}

#define cond_printf(__exp, __fmt, __args...) \
do { \
    if (__exp) \
        printf(__fmt, ##__args); \
} while (0)

#define dprintf(__fmt, __args...) cond_printf(debug_print, __fmt, ##__args)

#define eprint(__fmt, __args...) \
do { \
    fprintf(stderr, __fmt, ##__args); \
    exit(EXIT_FAILURE); \
} while (0)

template <typename T>
inline void *int2ptr(T i) {
    return (void *)(intptr_t(i));
}

template <typename T>
inline T ptr2int(void *p) {
    return (T)(intptr_t(p));
}

inline uint32_t rnd(uint32_t *seed) {
    *seed = *seed * 1103515245 + 12345;
    return *seed & 0x7fffffff;
}

inline uint64_t read_tsc(void) {
    uint32_t a, d;
    __asm __volatile("rdtsc":"=a"(a), "=d"(d));
    return ((uint64_t) a) | (((uint64_t) d) << 32);
}

inline uint64_t read_pmc(uint32_t ecx) {
    uint32_t a, d;
    __asm __volatile("rdpmc":"=a"(a), "=d"(d):"c"(ecx));
    return ((uint64_t) a) | (((uint64_t) d) << 32);
}

inline void mfence(void) {
    __asm __volatile("mfence");
}

inline void compiler_barrier() {
    __asm__ __volatile__("": : :"memory");
}

inline void nop_pause(void) {
    __asm __volatile("pause"::);
}

inline uint64_t usec(void) {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return uint64_t(tv.tv_sec) * 1000000 + tv.tv_usec;
}

inline uint64_t get_cpu_freq(void) {
#ifdef JOS_USER
    return 2000 * 1024 * 1024;
#else
    FILE *f = fopen("/proc/cpuinfo", "r");
    assert(f != NULL);
    float freqf = 0;
    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, f) != EOF &&
           sscanf(line, "cpu MHz\t: %f", &freqf) != 1);
    fclose(f);
    return uint64_t(freqf * (1 << 20));
#endif
}

inline uint64_t cycle_to_ms(uint64_t x) {
    return (x * 1000) / get_cpu_freq();
}

inline uint32_t get_core_count(void) {
    int r = sysconf(_SC_NPROCESSORS_ONLN);
    if (r < 0)
	eprint("get_core_count: error: %s\n", strerror(errno));
    return r;
}

inline int fill_core_array(uint32_t *cid, uint32_t n) {
    uint32_t z = get_core_count();
    if (n < z)
	return -1;

    for (uint32_t i = 0; i < z; ++i)
	cid[i] = i;
    return z;
}

inline void lfence(void) {
    __asm __volatile("lfence");
}

inline int atomic_add32_ret(int *cnt) {
    int __c = 1;
    __asm__ __volatile("lock; xadd %0,%1":"+r"(__c), "+m"(*cnt)::"memory");
    return __c;
}
#endif