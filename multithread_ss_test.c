#include <stdio.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include <x86intrin.h>

// This file is for running silent store tests on x86 machines
// Specifically uses hyperthreading amplification to have one child constantly reading memory address being written to in tests
// Functions are marked as noinline to force the compiler to not optimize code

// Note: this test is deprecated, as the newest version will be the silent_detect.c test

#define ARR_SIZE 512
uint16_t* tmp;
pthread_mutex_t lock;

void __attribute__ ((noinline)) read_data(void *data) {
    // read the
    pthread_mutex_lock(&lock);
    uint16_t prior = *((uint16_t *) data);
    pthread_mutex_unlock(&lock);
    while (1) {
        pthread_mutex_lock(&lock);
        uint16_t new_data = atomic_load(tmp);
        pthread_mutex_unlock(&lock);

        if (prior != new_data) {
            printf("Changed!\n");
        }
    }
}

// Flushes cache lines
void __attribute__ ((noinline)) flush_cache(void *p, unsigned int allocation) {
    unsigned long cache_line = 64;
    char *cp = (char *) p;
    unsigned long long i = 0;

    for (i = 0; i < allocation; i += cache_line) {
        _mm_clflush(&cp[i]);
    }
}

// Finds value of RDTSC command on x86
unsigned long long __attribute__ ((noinline)) rdtsc() {
    unsigned long long a, b;
    asm volatile("rdtsc\n\t"
                : "=a"(a), "=d"(b));
    
    return a | (b << 32);
}

// Run silent store or non-silent store test based on silent_store value (1 for silent store, 0 for non-silent store)
unsigned long long __attribute__ ((noinline)) run_test(int silent_store) {
    uint16_t load_target[ARR_SIZE];
    uint16_t *end = load_target + ARR_SIZE;
    
    // Create array and initialize all to zero if silent store test
    uint8_t zero = 0;
    uint16_t val;
    asm volatile("movzbw %1, %0\n\t"
                : "=r" (val)
                : "r" (zero));
    // Create array and initialize with increasing values if non-silent store test
    for (uint16_t *buf = load_target; buf < end; buf++) {
        *buf = val;
        if (!silent_store) {
            val++;
        }
    }
    
    // Ensure prior initialization is complete after this fence instruction
    _mm_mfence();
    
    // Flush all caches
    flush_cache(load_target, ARR_SIZE * sizeof(uint16_t));

    // Fence right before timing
    _mm_mfence();

    // Perform many writes to this variable
    unsigned long long start = rdtsc();
    for (uint16_t *buf = load_target; buf < end; buf++) {
        pthread_mutex_lock(&lock);
        *tmp = *buf;
        pthread_mutex_lqock(&lock);
    }
    
    // Ensure all store operations have completed
    _mm_mfence();

    return rdtsc() - start;
}

// Runs either silent store or non-silent store experiment (based on silent_store value) for num_cases amount
unsigned long long __attribute__ ((noinline)) run_experiment(unsigned int warmup, unsigned int cases, int silent_store) {
    // Starting warm-up phase for benchmark
    for (int i = 0; i < warmup; i++) {
        run_test(silent_store);
    }

    // Now warmed up, do actual test
    unsigned long long cycles = 0;
    for (int i = 0; i < cases; i++) {
        cycles += run_test(silent_store);
    }

    return cycles / cases;
}

int main() {
    tmp = malloc(sizeof(uint16_t));
    pthread_mutex_init(&lock, NULL);
    // Create thread running constantly to be checking tmp
    // Synchronize around access to tmp, use atomic load operation to get data
    
    unsigned long silent_cycles = run_experiment(100, 1000, 1);
    unsigned long non_silent_cycles = run_experiment(100, 1000, 0);

    printf("Average silent store cycles: %ld\n", silent_cycles);
    printf("Average non-silent store cycles: %ld\n", non_silent_cycles);
    return 0;
}
