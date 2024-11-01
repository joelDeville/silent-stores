#!/bin/bash

sudo bash -c "echo off > /sys/devices/system/cpu/smt/control" && \
sudo cpufreq-set -c 2-3 -g performance && \
sudo cpufreq-set -c 2-3 --min 2200MHz --max 2200MHz && \
sudo cset shield -c 2-3 -k on && \
sudo cset shield -e sudo -- -u "$USER" env "PATH=$PATH" bash
