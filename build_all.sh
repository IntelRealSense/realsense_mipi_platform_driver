#!/bin/bash

set -e

export DEVDIR=$(cd `dirname $0` && pwd)

export CROSS_COMPILE=aarch64-linux-gnu-
export LOCALVERSION=-tegra
export TEGRA_KERNEL_OUT=$DEVDIR/images
mkdir -p $TEGRA_KERNEL_OUT
export KERNEL_MODULES_OUT=$TEGRA_KERNEL_OUT/modules
cd $DEVDIR/../kernel/kernel-4.9
make ARCH=arm64 O=$TEGRA_KERNEL_OUT tegra_defconfig
make ARCH=arm64 O=$TEGRA_KERNEL_OUT -j`nproc`
make ARCH=arm64 O=$TEGRA_KERNEL_OUT modules_install INSTALL_MOD_PATH=$KERNEL_MODULES_OUT
