#include <stdio.h>
#include <stdint.h>

// This file is for running a silent store test on RISCV cores like Rocket Core and BOOM

#define ARR_SIZE 512

void flush_cache(void *p, unsigned int allocation) {
    unsigned long cache_line = 64;
    char *cp = (char *) p;
    unsigned long i = 0;

    for (i = 0; i < allocation; i += cache_line) {
        //_mm_clflush(&cp[i]);
        /*asm volatile("clflush (%0)\n\t"
                    :
                    : "r"(&cp[i])
                    : "memory");*/
    }
}

unsigned long rdcycle() {
    unsigned long lo = 0;
    asm volatile("rdcycle %0"
                : "=r"(lo));
    
    return lo;
}

unsigned long silent_store_test() {
    uint16_t load_target[ARR_SIZE];
    uint16_t *end = load_target + ARR_SIZE;
    
    // Create array and initialize all to zero
    uint16_t val = 0;
    asm volatile("li %0, 0\n\t"
                : "=r" (val));
    for (uint16_t *buf = load_target; buf < end; buf++) {
        *buf = val;
    }
    
    // Ensure prior initialization is complete after this fence instruction
    asm volatile("fence");
   
    // Flush all caches
    //flush_cache(load_target, ARR_SIZE * sizeof(uint16_t));

    // Fence right before timing
    asm volatile("fence");
    
    // Perform many writes to this variable
    volatile uint16_t tmp;
    unsigned long start = rdcycle();
    for (uint16_t *buf = load_target; buf < end; buf++) {
        tmp = *buf;
    }
    
    // Ensure all store operations have completed
    asm volatile("fence");

    return rdcycle() - start;
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
    asm volatile("fence");
    
    // Flush all caches
    //flush_cache(load_target, ARR_SIZE * sizeof(uint16_t));

    // Fence right before timing
    asm volatile("fence");

    // Perform many writes to this variable
    volatile uint16_t tmp;
    unsigned long start = rdcycle();
    for (uint16_t *buf = load_target; buf < end; buf++) {
        tmp = *buf;
    }
    
    // Ensure all store operations have completed
    asm volatile("fence");

    return rdcycle() - start;
}

// Runs silent_store experiment for num_cases amount
unsigned long run_silent_experiment(unsigned int warmup, unsigned int cases) {
    // Starting warm-up phase for benchmark
    for (int i = 0; i < warmup; i++) {
        silent_store_test();
    }

    // Now warmed up, do actual test
    unsigned long cycles = 0;
    for (int i = 0; i < cases; i++) {
        cycles += silent_store_test();
    }

    return cycles / cases;
}

unsigned long run_non_silent_experiment(unsigned int warmup, unsigned int cases) {
    // Starting warm-up phase for benchmark
    for (int i = 0; i < warmup; i++) {
        non_silent_store_test();
    }

    // Now warmed up, do actual test
    unsigned long cycles = 0;
    for (int i = 0; i < cases; i++) {
        cycles += non_silent_store_test();
    }

    return cycles / cases;
}

int main() {
    unsigned long silent_cycles = run_silent_experiment(10, 10);
    unsigned long non_silent_cycles = run_non_silent_experiment(10, 10);

    printf("Average silent store cycles: %ld\n", silent_cycles);
    printf("Average non-silent store cycles: %ld\n", non_silent_cycles);
    return 0;
}
