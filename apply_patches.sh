#!/bin/bash

set -e

if [ $# != 1 ]; then
    echo "1st arg should be 'apply' or 'reset'"
    exit 1
fi

apply_external_patches() {
    if [ $1 = 'apply' ]; then
        git -C ../$2 am ${PWD}/$2/*
    elif [ $1 = 'reset' ]; then
        git -C ../$2 reset --hard tegra-l4t-r32.6.1
    fi
}

apply_external_patches $1 kernel/nvidia
apply_external_patches $1 kernel/kernel-4.9
apply_external_patches $1 hardware/nvidia/platform/t19x/galen/kernel-dts

