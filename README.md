This project contains the test program and other content for my final project in CS 350C (Advanced Computer Architecture) taken at the University of Texas at Austin.

To compile the test, use 'gcc -o ss -msse2 ss_test.c', as this will let the compiler know what specific target to compile for.

For accurate benchmarking results, follow below details.

Disable hyperthreading:
- sudo bash -c "echo off > /sys/devices/system/cpu/smt/control"
- do 'cat /proc/cpuinfo' to ensure legitimate cores are being used in test after disabling hyperthreading'

Disable frequency scaling
set cpu freq on CPU 2
- sudo cpufreq-set -c 2 -g performance
- sudo cpufreq-set -c 2 --min 2200MHz --max 2200MHz

Disable swapping of cores for this task
set cpu shield on CPU 2
- sudo cset shield -c 2 -k on
- sudo cset shield -e sudo -- -u "$USER" env "PATH=$PATH" bash

Details below are to re-enable the features disabled above.
You are in a subshell now, run your benchmark here
Ctrl+D to close the current subshell

Enable hyperthreading
- sudo bash -c "echo on > /sys/devices/system/cpu/smt/control"

Reset cpu frequency on CPU 2 by copying policy from cpu 0
- POLICYINFO=($(cpufreq-info -c 0 -p)) && \
- sudo cpufreq-set -c 2 -g ${POLICYINFO[2]} && \
- sudo cpufreq-set -c 2 --min ${POLICYINFO[0]}MHz --max ${POLICYINFO[1]}MHz

Make sure to check if machine allows for hyperthreading and to try to locate hyperthreaded cores
- likely they will be sibling cores
- join threads to those cores
In general, remember cache states to notice effects on timing differences (exclusivity & shared states)
Use atomic variable types to ensure reading/writing is atomic always in a test
Never print within a test as it uses many subsystems. Instead, place something into memory and check it after test has ran.
- For instance, using int[] to add elements into and analyze elements after test has finished

For maximum information gathering, run this test on as many machines as possible (Intel, Arm, M1, etc)

TODO: next steps: cache line flushes potentially during test
flush store buffers (somehow) so cache coherency kicks in to affect timing differences
