// This test is the updated test to determine if the silent store optimization exists on whichever machine it runs on
// Idea is: if hyperthread takes longer, this will be on a non-silent store machine due to cache states
// Keep in mind store elision, where compiler can optimize out earlier stores to the same location and only take last store
// This can be determined by writing one single value to location multiple times instead of different values, not necessarily focus of this test though
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

// Thread function to continuously read variable
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
    unsigned long long start = rdtsc();

    return rdtsc() - start;
}

int main() {
    tmp = malloc(sizeof(uint16_t));
    printf("Cycles taken: %lld\n", run_test());
    free(tmp);
    return 0;
}
