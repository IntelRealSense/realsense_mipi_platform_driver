#!/bin/bash

set -e

if [[ $# < 1 ]]; then
    echo "apply_patches.sh [--one-cam] apply [JetPack_version]"
    echo "apply_patches.sh reset [JetPack_version]"
    exit 1
fi

# Default to dual camera DT for JetPack 5.0.2
JP5_D4XX_DTSI="tegra194-camera-d4xx.dtsi"
if [[ "$1" == "--one-cam" ]]; then
    JP5_D4XX_DTSI="tegra194-camera-d4xx-single.dtsi"
    shift
fi

DEVDIR=$(cd `dirname $0` && pwd)

. $DEVDIR/scripts/setup-common "$2"

cd "$DEVDIR"

# NVIDIA SDK Manager's JetPack 4.6.1 source_sync.sh doesn't set the right folder name, it mismatches with the direct tar
# package source code. Correct the folder name.
if [ -d sources_$JETPACK_VERSION/hardware/nvidia/platform/t19x/galen-industrial-dts ]; then
    mv sources_$JETPACK_VERSION/hardware/nvidia/platform/t19x/galen-industrial-dts sources_$JETPACK_VERSION/hardware/nvidia/platform/t19x/galen-industrial
fi

apply_external_patches() {
    if [ $1 = 'apply' ]; then
        git -C sources_$JETPACK_VERSION/$2 am ${PWD}/$2/$JETPACK_VERSION/*
    elif [ $1 = 'reset' ]; then
        git -C sources_$JETPACK_VERSION/$2 reset --hard $L4T_VERSION
    fi
}

apply_external_patches $1 kernel/nvidia
apply_external_patches $1 $KERNEL_DIR
apply_external_patches $1 hardware/nvidia/platform/t19x/galen/kernel-dts

if [ $1 = 'apply' ]; then
    cp $DEVDIR/d4xx.c $DEVDIR/sources_$JETPACK_VERSION/kernel/nvidia/drivers/media/i2c/
    cp $DEVDIR/$JP5_D4XX_DTSI $DEVDIR/sources_$JETPACK_VERSION/hardware/nvidia/platform/t19x/galen/kernel-dts/common/tegra194-camera-d4xx.dtsi
elif [ $1 = 'reset' ]; then
    rm $DEVDIR/sources_$JETPACK_VERSION/kernel/nvidia/drivers/media/i2c/d4xx.c
fi
