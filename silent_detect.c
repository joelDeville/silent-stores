#define _GNU_SOURCE 
#include <stdatomic.h>  // useful for having atomic variables
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>

// This test is the updated test to determine if the silent store optimization exists on whichever machine it runs on
// Idea is: if hyperthread takes longer, this will be on a non-silent store machine due to cache states
// Keep in mind store elision, where compiler can optimize out earlier stores to the same location and only take last store
// This can be determined by writing one single value to location multiple times instead of different values, not necessarily focus of this test though

#define ARR_SIZE 128
#define FORCE_READ_INT(var) __asm__("" ::"r"(var))
// Force read defined above to have compiler not optimize out unused read

uint16_t *tmp;

// Functions are marked as noinline to force the compiler to not optimize code

// Thread function to continuously read variable
void * __attribute__ ((noinline)) read_var(void *p) {
    // Continuously read 
    while (1) {
        FORCE_READ_INT(tmp);
    }
}

void * __attribute__ ((noinline)) write_var(void *p) {
    // Continuously write (must be different value than main thread is writing to same location)
    int val_to_write = 5;
    while (1) {
        *tmp = val_to_write;
    }
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
    int val_to_write = 4;
    for (uint16_t i = 0; i < 1000; i++) {
        *tmp = val_to_write;
    }
    return rdtsc() - start;
}

// Runs either silent store or non-silent store experiment (based on silent_store value) for num_cases amount
// reading var will be 1 if non-main thread will be solely reading and 0 if solely writing to data location
unsigned long long __attribute__ ((noinline)) run_experiment(unsigned int warmup, unsigned int cases, int reading) {
    // Create additional thread
    pthread_attr_t attr;
    pthread_t tid;
    pthread_attr_init(&attr);
    cpu_set_t cpu_set;
    int thread_cpu = 1; // Change this to other thread shielded before running program
    CPU_ZERO(&cpu_set);
    CPU_SET(thread_cpu, &cpu_set);
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set);
    if (reading) {
        pthread_create(&tid, &attr, read_var, NULL);
    } else {
        pthread_create(&tid, &attr, write_var, NULL);
    }

    // Starting warm-up phase for benchmark
    for (int i = 0; i < warmup; i++) {
        run_test();
    }

    // Now warmed up, do actual test
    unsigned long long cycles = 0;
    for (int i = 0; i < cases; i++) {
        cycles += run_test();
    }

    return cycles / cases;
}

int main() {
    tmp = malloc(sizeof(uint16_t));
    // Create structure for CPUs on system, zero them out then add one of them to the set for the main thread
    cpu_set_t cst;
    int cpu_desired = 0; // Change this to either CPU that is shielded before running this program
    CPU_ZERO(&cst);
    CPU_SET(cpu_desired, &cst);

    printf("Read cycles taken: %lld\n", run_experiment(100, 1000, 1));
    printf("Write cycles taken: %lld\n", run_experiment(100, 1000, 0));

    free(tmp);
    return 0;
}
