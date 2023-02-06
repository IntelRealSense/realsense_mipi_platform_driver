#!/bin/bash
set -e

if [[ $# < 1 ]]; then
    echo "apply_patches_ext.sh [--one-cam | --dual-cam] source_dir [JetPack_version]"
    exit 1
fi

# Default to single camera DT for JetPack 5.0.2
# single - dev board
# one/dual - evb
JP5_D4XX_DTSI="tegra194-camera-d4xx.dtsi"
if [[ "$1" == "--one-cam" ]]; then
    JP5_D4XX_DTSI="tegra194-camera-d4xx-single.dtsi"
    shift
fi
if [[ "$1" == "--dual-cam" ]]; then
    JP5_D4XX_DTSI="tegra194-camera-d4xx-dual.dtsi"
    shift
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

# For a common driver for JP4 + JP5 we override the i2c driver and ignore the previous that was created from patches
cp $DEVDIR/d4xx.c $DEVDIR/$1/kernel/nvidia/drivers/media/i2c/
cp $DEVDIR/hardware/realsense/$JP5_D4XX_DTSI $DEVDIR/$1/hardware/nvidia/platform/t19x/galen/kernel-dts/common/tegra194-camera-d4xx.dtsi
