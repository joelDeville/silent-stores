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

unsigned long silent_store_test() {
    uint16_t load_target[ARR_SIZE];
    uint16_t *end = load_target + ARR_SIZE;
    
    // Create array and initialize all to zero
    uint8_t zero = 0;
    uint16_t val;
    asm volatile("movzbw %1, %0\n\t"
                : "=r" (val)
                : "r" (zero));
    for (uint16_t *buf = load_target; buf < end; buf++) {
        *buf = 0;
    }
    
    // Ensure prior initialization is complete after this fence instruction
    _mm_mfence();
    
    // Flush all caches
    flush_cache(load_target, ARR_SIZE * sizeof(uint16_t));

    // Fence right before timing
    _mm_mfence();

    // Start timing operation
    unsigned long long start = _rdtsc();
    
    // Perform many writes to this variable
    volatile uint16_t tmp;
    for (uint16_t *buf = load_target; buf < end; buf++) {
        tmp = *buf;
    }
    
    // Ensure all store operations have completed
    _mm_mfence();
    
    // Stop timing
    unsigned long long stop = _rdtsc();

    //printf("Silent test took %d\n", stop - start);
    return stop - start;
}

unsigned long non_silent_store_test() {
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

    // Start timing operation
    unsigned long long start = _rdtsc();
    
    // Perform many writes to this variable
    volatile uint16_t tmp;
    for (uint16_t *buf = load_target; buf < end; buf++) {
        tmp = *buf;
    }
    
    // Ensure all store operations have completed
    _mm_mfence();
    
    // Stop timing
    unsigned long long stop = _rdtsc();

    //printf("Non-silent test took %d\n", stop - start);
    return stop - start;
}

int main() {
    int num_cases = 10;
    long total_cycles = 0;

    for (int i = 0; i < num_cases; i++) {
        total_cycles += silent_store_test();
    }

    printf("Finished silent tests\nTook average of %ld cycles\n", total_cycles / num_cases);
    total_cycles = 0;

    for (int i = 0; i < num_cases; i++) {
        total_cycles += non_silent_store_test();
    }

    printf("Finished non-silent tests\nTook average %ld cycles\n", total_cycles / num_cases);
}
