#!/bin/bash
set -e

if [[ $# < 1 ]]; then
    echo "apply_patches_ext.sh source_dir [JetPack_version]"
    exit 1
fi

DEVDIR=$(cd `dirname $0` && pwd)

. $DEVDIR/scripts/setup-common "$2"

cd "$DEVDIR"

# NVIDIA SDK Manager's JetPack 4.6.1 source_sync.sh doesn't set the right folder name, it mismatches with the direct tar
# package source code. Correct the folder name.
if [ -d $1/hardware/nvidia/platform/t19x/galen-industrial-dts ]; then
    mv $1/hardware/nvidia/platform/t19x/galen-industrial-dts $1/hardware/nvidia/platform/t19x/galen-industrial
fi

apply_external_patches() {
cat ${PWD}/$2/$JETPACK_VERSION/* | patch -p1 --directory=${PWD}/$1/$2/
}

apply_external_patches $1 kernel/nvidia
apply_external_patches $1 $KERNEL_DIR
apply_external_patches $1 hardware/nvidia/platform/t19x/galen/kernel-dts

if [[ $JETPACK_VERSION =~ 5.* ]]; then
    cp $DEVDIR/d4xx.c $DEVDIR/$1/kernel/nvidia/drivers/media/i2c/
fi
