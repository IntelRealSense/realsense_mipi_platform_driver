#!/bin/bash

set -e

function DisplayNvidiaLicense {

    # verify that curl is installed
    if  ! which curl > /dev/null  ; then
      echo "curl is not installed."
      echo "curl can be installed by 'sudo apt-get install curl'."
      exit 1
    fi
    
    echo -e "\nPlease notice: This script will download the kernel source (from nv-tegra, NVIDIA's public git repository) which is subject to the following license:\n\nhttps://developer.nvidia.com/embedded/l4t/r35_release_v1.0/release/tegra_software_license_agreement-tegra-linux.txt\n"

    # license="$(curl -L -s https://developer.nvidia.com/embedded/l4t/r35_release_v1.0/release/tegra_software_license_agreement-tegra-linux.txt)\n\n"
    license="$(curl -L -s https://developer.download.nvidia.com/embedded/L4T/r35_Release_v4.1/release/Tegra_Software_License_Agreement-Tegra-Linux.txt)\n\n"

    ## display the page ##
    echo -e "${license}"

    read -t 30 -n 1 -s -r -e -p 'Press any key to continue (or wait 30 seconds..)'
}


if [[ "$1" == "-h" ]]; then
    echo "setup_workspace.sh [JetPack_version]"
    echo "setup_workspace.sh -h"
    echo "JetPack_version can be 4.6.1, 5.0.2, 5.1.2, 6.0"
    exit 1
fi

export DEVDIR=$(cd `dirname $0` && pwd)

. $DEVDIR/scripts/setup-common "$1"
echo "Setup JetPack $JETPACK_VERSION to sources_$JETPACK_VERSION"

# Display NVIDIA license
DisplayNvidiaLicense ""

# Install L4T gcc if not installed
if [[ ! -d "$DEVDIR/l4t-gcc/$JETPACK_VERSION/bin/" ]]; then
    mkdir -p $DEVDIR/l4t-gcc/$JETPACK_VERSION
    cd $DEVDIR/l4t-gcc/$JETPACK_VERSION
    if [[ "$JETPACK_VERSION" == "6.0" ]]; then
        wget https://developer.nvidia.com/downloads/embedded/l4t/r36_release_v2.0/toolchain/aarch64--glibc--stable-2022.08-1.tar.bz2 -O aarch64--glibc--stable-final.tar.bz2
        tar xf aarch64--glibc--stable-final.tar.bz2 --strip-components 1
    elif [[ "$JETPACK_VERSION" == "5.1.2" ]]; then
        wget https://developer.nvidia.com/embedded/jetson-linux/bootlin-toolchain-gcc-93 -O aarch64--glibc--stable-final.tar.gz
        tar xf aarch64--glibc--stable-final.tar.gz
    elif [[ "$JETPACK_VERSION" == "5.0.2" ]]; then
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

# copy Makefile for jp6
if [[ "$JETPACK_VERSION" == "6.0" ]]; then
    cp ./nvidia-oot/Makefile ./sources_$JETPACK_VERSION/
    cp ./kernel/kernel-jammy-src/Makefile ./sources_$JETPACK_VERSION/kernel
fi

# remove BUILD_NUMBER env dependency kernel vermagic
if [[ "${JETPACK_VERSION}" == "4.6.1" ]]; then
    sed -i s/'UTS_RELEASE=\$(KERNELRELEASE)-ab\$(BUILD_NUMBER)'/'UTS_RELEASE=\$(KERNELRELEASE)'/g ./sources_$JETPACK_VERSION/kernel/kernel-4.9/Makefile
    sed -i 's/the-space :=/E =/g' ./sources_$JETPACK_VERSION/kernel/kernel-4.9/scripts/Kbuild.include
    sed -i 's/the-space += /the-space = \$E \$E/g' ./sources_$JETPACK_VERSION/kernel/kernel-4.9/scripts/Kbuild.include
fi
