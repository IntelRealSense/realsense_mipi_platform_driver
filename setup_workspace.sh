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

    license="$(curl -L -s https://developer.nvidia.com/embedded/l4t/r35_release_v1.0/release/tegra_software_license_agreement-tegra-linux.txt)\n\n"
    ## display the page ##
    echo -e "${license}"

    read -t 30 -n 1 -s -r -e -p 'Press any key to continue (or wait 30 seconds..)'
}


if [[ "$1" == "-h" ]]; then
    echo "setup_workspace.sh [JetPack_version]"
    echo "setup_workspace.sh -h"
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

