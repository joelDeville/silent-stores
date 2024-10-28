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

// Force read defined to have compiler not optimize out unused read
#define FORCE_READ_INT(var) __asm__("" ::"r"(var))
#define WARMUP_PERIOD 100
#define NUM_CASES 1000
// Set below variables to make sure thread are on shielded cores
// Note: check these cores are actually legitimate after disabling hyperthreading
#define MAIN_SHIELDED_CORE 0
#define OTHER_SHIELDED_CORE 1

uint16_t *tmp;

// Thread function to continuously read variable
void * read_var(void *p) {
    // Continuously read 
    while (1) {
        FORCE_READ_INT(tmp);
    }
}

// Finds value of RDTSC command on x86
unsigned long long rdtsc() {
    unsigned long long a, b;
    asm volatile("rdtsc\n\t"
                : "=a"(a), "=d"(b));
    
    return a | (b << 32);
}

unsigned long long run_test(int write_diff_value) {
    unsigned long long start = rdtsc();
    static int val_to_write = 4;
    *tmp = val_to_write;
    if (write_diff_value) {
        val_to_write++;
    }
    return rdtsc() - start;
}

// Runs test for given warmup period and cases
unsigned long long run_experiment(unsigned int warm_up, unsigned int cases, int write_diff_value) {
    // Starting warm-up phase for benchmark
    for (int i = 0; i < warm_up; i++) {
        run_test(write_diff_value);
    }

    // Now warmed up, do actual test
    unsigned long long cycles = 0;
    for (int i = 0; i < cases; i++) {
        cycles += run_test(write_diff_value);
    }

    return cycles / cases;
}

int configure_threads() {
    // Create additional thread to continuously read data location
    pthread_attr_t attr;
    pthread_t tid;
    int ret = pthread_attr_init(&attr);
    if (ret) {
        printf("Error with setting default attributes of new thread\n");
        return 1;
    }
    cpu_set_t cpu_set;
    int thread_cpu = OTHER_SHIELDED_CORE;
    CPU_ZERO(&cpu_set);
    CPU_SET(thread_cpu, &cpu_set);
    // Set affinity other thread to diff. shielded core
    ret = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set), &cpu_set);
    if (ret) {
        printf("Error with setting affinity in thread attribute\n");
        return 1;
    }
    ret = pthread_create(&tid, &attr, read_var, NULL);
    if (ret) {
        printf("Error creating additional thread\n");
        return 1;
    }

    // Create structure for CPUs on system, zero them out then add one of them to the set for the main thread
    int cpu_desired = MAIN_SHIELDED_CORE;
    CPU_ZERO(&cpu_set);
    CPU_SET(cpu_desired, &cpu_set);
    ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set);
    if (ret) {
        printf("Error setting affinity of main thread\n");
        return 1;
    }
}

int main() {
    // Allocate variable in heap and configure main/additional thread
    tmp = malloc(sizeof(uint16_t));
    if (tmp == NULL) {
        printf("Unable to allocate heap memory\n");
        return 1;
    }
    if (configure_threads()) {
        return 1;
    }

    // Run test with thread in background continuously reading, one for writing same value to one location and another for writing diff. values
    printf("Writing different values cycles taken: %lld\n", run_experiment(100, 1000, 1));
    printf("Writing same value cycles taken: %lld\n", run_experiment(100, 1000, 0));

    free(tmp);
    return 0;
}
