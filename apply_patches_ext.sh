#!/bin/bash
set -e
set -x

# NVIDIA SDK Manager's JetPack 4.6.1 source_sync.sh doesn't set the right folder name, it mismatches with the direct tar
# package source code. Correct the folder name.
if [ -d $1/hardware/nvidia/platform/t19x/galen-industrial-dts ]; then
    mv $1/hardware/nvidia/platform/t19x/galen-industrial-dts $1/hardware/nvidia/platform/t19x/galen-industrial
fi

apply_external_patches() {
cat ${PWD}/$2/* | patch -p1 --directory=${PWD}/$1/$2/
}

apply_external_patches $1 kernel/nvidia
apply_external_patches $1 kernel/kernel-4.9
apply_external_patches $1 hardware/nvidia/platform/t19x/galen/kernel-dts
cp d4xx.c ./$1/kernel/nvidia/drivers/media/i2c/ 
