#!/bin/bash

set -e

if [[ "$1" == "-h" ]]; then
    echo "setup_workspace.sh [JetPack_version]"
    echo "setup_workspace.sh -h"
    exit 1
fi

export DEVDIR=$(cd `dirname $0` && pwd)

. $DEVDIR/scripts/setup-common "$1"
echo "Setup JetPack $JETPACK_VERSION to sources_$JETPACK_VERSION"

# Install L4T gcc if not installed
if [[ ! -d "$DEVDIR/l4t-gcc/$JETPACK_VERSION/bin/" ]]; then
    mkdir -p $DEVDIR/l4t-gcc/$JETPACK_VERSION
    cd $DEVDIR/l4t-gcc/$JETPACK_VERSION
    if [[ "$JETPACK_VERSION" == "5.0.2" ]]; then
        wget https://developer.nvidia.com/embedded/jetson-linux/bootlin-toolchain-gcc-93 -O aarch64--glibc--stable-final.tar.gz
        tar xf aarch64--glibc--stable-final.tar.gz
    elif [[ "$JETPACK_VERSION" == "4.6.1" ]]; then
        wget http://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/aarch64-linux-gnu/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu.tar.xz
        tar xf gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu.tar.xz --strip-components 1
    fi
fi

# Clone L4T kernel source repo
cd $DEVDIR
./scripts/source_sync_$JETPACK_VERSION.sh -t $L4T_VERSION -d sources_$JETPACK_VERSION
