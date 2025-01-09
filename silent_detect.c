#define _GNU_SOURCE 
#include <stdatomic.h>  // useful for having atomic variables
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>

// This test is the updated test to determine if the silent store optimization exists on whichever machine it runs on
// Idea is: if main thread writing one value continuously and thread continuously reading takes longer than writing different values, then silent stores exist
// Keep in mind store elision, where compiler can optimize out earlier stores to the same location and only take last store
// This can be determined by writing one single value to location multiple times instead of different values, not necessarily focus of this test though

// Force read defined to have compiler not optimize out unused read
#define FORCE_READ_INT(var) __asm__("" ::"r"(var))
#define WARMUP_PERIOD 10000
#define NUM_CASES 100000
// Set below variables to make sure threads are on shielded cores
// Note: check these cores are actually legitimate after disabling hyperthreading
#define MAIN_SHIELDED_CORE 2
#define OTHER_SHIELDED_CORE 3

// Checking and creating different syntax for instructions based on architecture
#if defined(__arm__ ) || defined(__aarch64__)
    #define X86 0
    #define GET_TIME(a, b)  asm volatile("mrs %0, cntvct_el0" : "=r" (*a));
    #define XOR_VALUE(a, b) asm volatile("eor %[out], %[out], %[in]" : [out] "+r" (*a) : [in] "r" (*b));
#elif defined(__x86_64)
    #define X86 1
    #define GET_TIME(a, b) asm volatile("rdtsc" : "=a"(*a), "=d"(*b));
    #define XOR_VALUE(a, b) asm volatile("xor %[in], %[out]" : [out] "+r" (*a) : [in] "r" (*b));
    #define INC_VALUE(a) asm volatile("inc %[inout]" : [inout] "+r" (*a));
// #elif defined(ANDROID) TODO: find Android API to create assembly calls for it
#endif
// Note: r means putting variable into register, + means reading/writing and cI means immediate or using CL register for shift amount

// Variable in heap being written to
uint16_t *tmp;
// global int for xor-ing/rotating with val to write to heap
// Say x is value written to heap, set xor_val to 0 to keep value the same
// Set to anything else to alternate values written to heap
int asm_val = 0;

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
    unsigned long long result;
    if (X86) {
        GET_TIME(&a, &b);
        result = a | (b << 32);
    } else {
        GET_TIME(&a, &b);
        result = a;
    }
    return result;
}

// Runs test for given warmup period and cases
unsigned long long run_experiment(unsigned int warm_up, unsigned int cases, int write_diff_value) {
    // Starting warm-up phase for benchmark
    int val_to_write = 4;
    for (int i = 0; i < warm_up; i++) {
        *tmp = val_to_write;
        XOR_VALUE(&val_to_write, &asm_val);
    }

    // Now warmed up, do actual test
    unsigned long long start = rdtsc();
    for (int i = 0; i < cases; i++) {
        *tmp = val_to_write;
        XOR_VALUE(&val_to_write, &asm_val);
    }
    return rdtsc() - start;
}

int configure_threads() {
    // Create additional thread to continuously read data location
    pthread_attr_t attr;
    pthread_t tid;
    int ret = pthread_attr_init(&attr);
    if (ret) {
        printf("Error with setting default attributes of new thread: %d\n", ret);
        return 1;
    }
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(OTHER_SHIELDED_CORE, &cpu_set);
    // Set affinity other thread to diff. shielded core
    ret = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set), &cpu_set);
    if (ret) {
        printf("Error with setting affinity in thread attribute: %d\n", ret);
        return 1;
    }
    ret = pthread_create(&tid, &attr, read_var, NULL);
    if (ret) {
        printf("Error creating additional thread: %d\n", ret);
        return 1;
    }

    // Create structure for CPUs on system, zero them out then add one of them to the set for the main thread
    CPU_ZERO(&cpu_set);
    CPU_SET(MAIN_SHIELDED_CORE, &cpu_set);
    ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set);
    if (ret) {
        printf("Error setting affinity of main thread: %d\n", ret);
        return 1;
    }
    return 0;
}

int main() {
    // Initialize log for writing data from run
    char *file_name = (X86) ? "logs/X86.txt" : "logs/ARM.txt";
    FILE* file = fopen(file_name, "a");
    fprintf(file, "Main thread isolated on core %d\n", MAIN_SHIELDED_CORE);
    fprintf(file, "Reading thread isolated on core %d\n", OTHER_SHIELDED_CORE);

    // Allocate variable in heap and configure main/additional thread
    tmp = malloc(sizeof(uint16_t));
    if (tmp == NULL) {
        printf("Unable to allocate heap memory\n");
        return 1;
    }
    if (configure_threads()) {
        printf("Error configuring threads\n");
        return 1;
    }

    // Run test with thread in background continuously reading, one for writing same value to one location and another for writing diff. values
    asm_val = 3;
    fprintf(file, "Writing diff. vals xor, cycles taken: %lld\n", run_experiment(WARMUP_PERIOD, NUM_CASES, 1));
    asm_val = 0;
    fprintf(file, "Writing same val xor, cycles taken: %lld\n\n", run_experiment(WARMUP_PERIOD, NUM_CASES, 0));
    printf("Done\n");
    free(tmp);
    fclose(file);
    return 0;
}
