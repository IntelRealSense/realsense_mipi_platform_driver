#!/bin/bash

set -e

if [ $# != 1 ]; then
    echo "1st arg should be 'apply' or 'reset'"
    exit 1
fi

# NVIDIA SDK Manager's JetPack 4.6.1 source_sync.sh doesn't set the right folder name, it mismatches with the direct tar
# package source code. Correct the folder name.
if [ -d ../hardware/nvidia/platform/t19x/galen-industrial-dts ]; then
    mv ../hardware/nvidia/platform/t19x/galen-industrial-dts ../hardware/nvidia/platform/t19x/galen-industrial
fi

apply_external_patches() {
    if [ $1 = 'apply' ]; then
        git -C ../$2 am ${PWD}/$2/*
    elif [ $1 = 'reset' ]; then
        git -C ../$2 reset --hard tegra-l4t-r32.7.1
    fi
}

apply_external_patches $1 kernel/nvidia
apply_external_patches $1 kernel/kernel-4.9
apply_external_patches $1 hardware/nvidia/platform/t19x/galen/kernel-dts

