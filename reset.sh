#!/bin/bash

sudo bash -c "echo on > /sys/devices/system/cpu/smt/control" && \
POLICYINFO=($(cpufreq-info -c 4 -p)) && \
sudo cpufreq-set -c 2-3 -g ${POLICYINFO[2]} && \
sudo cpufreq-set -c 2-3 --min ${POLICYINFO[0]}MHz --max ${POLICYINFO[1]}MHz && \
