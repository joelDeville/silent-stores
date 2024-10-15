// This test is the updated test to determine if the silent store optimization exists on whichever machine it runs on
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define ARR_SIZE 128
#define FORCE_READ_INT(var) __asm__("" ::"r"(var))
// Force read defined above to have compiler not optimize out unused read

atomic_int test_check[ARR_SIZE];
uint16_t *tmp;

// Functions are marked as noinline to force the compiler to not optimize code
void __attribute__ ((noinline)) read_var(void *p) {
    
}

// Finds value of RDTSC command on x86
unsigned long long __attribute__ ((noinline)) rdtsc() {
    unsigned long long a, b;
    asm volatile("rdtsc\n\t"
                : "=a"(a), "=d"(b));
    
    return a | (b << 32);
}

unsigned long long run_test() {
    // Create array and initialize with increasing values if non-silent store test
    // for (uint16_t *buf = load_target; buf < end; buf++) {
    //     *buf = 0;
    // }

    // // Perform many writes to this variable
    unsigned long long start = rdtsc();
    // for (uint16_t *buf = load_target; buf < end; buf++) {
    //     *tmp = *buf;
    // }

    return rdtsc() - start;
}

int main() {
    tmp = malloc(sizeof(uint16_t));
    printf("Cycles taken: %lld\n", run_test());
    free(tmp);
    return 0;
}
