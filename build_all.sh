#!/bin/bash

set -e

if [[ "$1" == "-h" ]]; then
    echo "build_all.sh [JetPack_version] [JetPack_Linux_source]"
    echo "build_all.sh -h"
    exit 1
fi

export DEVDIR=$(cd `dirname $0` && pwd)

. $DEVDIR/scripts/setup-common "$1"

SRCS="$DEVDIR/sources_$JETPACK_VERSION"
if [[ -n "$2" ]]; then
    SRCS="$2"
fi

if [[ "$JETPACK_VERSION" == "5.0.2" ]]; then
    export CROSS_COMPILE=$DEVDIR/l4t-gcc/$JETPACK_VERSION/bin/aarch64-buildroot-linux-gnu-
elif [[ "$JETPACK_VERSION" == "4.6.1" ]]; then
    export CROSS_COMPILE=$DEVDIR/l4t-gcc/$JETPACK_VERSION/bin/aarch64-linux-gnu-
fi
export LOCALVERSION=-tegra
export TEGRA_KERNEL_OUT=$DEVDIR/images/$JETPACK_VERSION
mkdir -p $TEGRA_KERNEL_OUT
export KERNEL_MODULES_OUT=$TEGRA_KERNEL_OUT/modules

cd $SRCS/$KERNEL_DIR

make ARCH=arm64 O=$TEGRA_KERNEL_OUT tegra_defconfig
make ARCH=arm64 O=$TEGRA_KERNEL_OUT -j`nproc`
make ARCH=arm64 O=$TEGRA_KERNEL_OUT modules_install INSTALL_MOD_PATH=$KERNEL_MODULES_OUT -j`nproc`
