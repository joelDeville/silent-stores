#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>

#define ARR_SIZE 512

void flush_cache(void *p, unsigned int allocation) {
    unsigned long cache_line = 64;
    char *cp = (char *) p;
    unsigned long i = 0;

    for (i = 0; i < allocation; i += cache_line) {
        _mm_clflush(&cp[i]);
    }
}

void silent_store_test() {
    uint16_t load_target[ARR_SIZE];
    uint16_t *end = load_target + ARR_SIZE;
    
    // Create array and initialize all to zero
    uint8_t zero = 0;
    uint16_t val;
    asm volatile("movzbw %1, %0\n\t"
                : "=r" (val)
                : "r" (zero));
    for (uint16_t *buf = load_target; buf < end; buf++) {
        *buf = val;
    }
    
    // Ensure prior initialization is complete after this fence instruction
    _mm_mfence();
    
    // Flush all caches
    flush_cache(load_target, ARR_SIZE * sizeof(uint16_t));

    // Fence right before timing
    _mm_mfence();
    
    // Perform many writes to this variable
    volatile uint16_t tmp;
    for (uint16_t *buf = load_target; buf < end; buf++) {
        tmp = *buf;
    }
    
    // Ensure all store operations have completed
    _mm_mfence();
}

void non_silent_store_test() {
    uint32_t val = 0;
    uint16_t load_target[ARR_SIZE];
    uint16_t *end = load_target + ARR_SIZE;
    
    // Create array and initialize with increasing values
    for (uint16_t *buf = load_target; buf < end; buf++) {
        *buf = val;
        val++;
    }
    
    // Ensure prior initialization is complete after this fence instruction
    _mm_mfence();
    
    // Flush all caches
    flush_cache(load_target, ARR_SIZE * sizeof(uint16_t));

    // Fence right before timing
    _mm_mfence();

    // Perform many writes to this variable
    volatile uint16_t tmp;
    for (uint16_t *buf = load_target; buf < end; buf++) {
        tmp = *buf;
    }
    
    // Ensure all store operations have completed
    _mm_mfence();
}

// Runs silent_store experiment for num_cases amount
unsigned long run_silent_experiment(unsigned int warmup, unsigned int cases) {
    // Starting warm-up phase for benchmark
    for (int i = 0; i < warmup; i++) {
        silent_store_test();
    }

    // Now warmed up, do actual test
    unsigned long start = _rdtsc();
    for (int i = 0; i < cases; i++) {
        silent_store_test();
    }
    unsigned long stop = _rdtsc();

    return (stop - start) / cases;
}

unsigned long run_non_silent_experiment(unsigned int warmup, unsigned int cases) {
    // Starting warm-up phase for benchmark
    for (int i = 0; i < warmup; i++) {
        non_silent_store_test();
    }

    // Now warmed up, do actual test
    unsigned long start = _rdtsc();
    for (int i = 0; i < cases; i++) {
        non_silent_store_test();
    }
    unsigned long stop = _rdtsc();

    return (stop - start) / cases;
}

int main() {
    unsigned long silent_cycles = run_silent_experiment(100, 100000);
    unsigned long non_silent_cycles = run_non_silent_experiment(100, 100000);

    printf("Average silent store cycles: %ld\n", silent_cycles);
    printf("Average non-silent store cycles: %ld\n", non_silent_cycles);
    return 0;
}
