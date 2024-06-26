# Intel® RealSense™ camera driver for GMSL* interface

# D457 MIPI on Jetson AGX Xavier
The RealSense MIPI platform driver enables the user to control and stream RealSense 3D MIPI cameras.
The system shall include:
* Jetson platform (Currently Supported JetPack versions are: 6.0, 5.1.2, 5.0.2, 4.6.1)
* RealSense De-Serialize board (https://store.intelrealsense.com/buy-intel-realsense-des457.html)
* RS MIPI camera (e.g. https://store.intelrealsense.com/buy-intel-realsense-depth-camera-d457.html)

> Note: This MIPI reference driver is based on RealSense de-serialize board. For other de-serialize boards, modification might be needed. 

![image](https://user-images.githubusercontent.com/64067618/216807681-ed679a79-71d6-43ab-bfde-e0abb019b72d.png)


## Jetson AGX Xavier board setup

Please follow the [instruction](https://docs.nvidia.com/sdk-manager/install-with-sdkm-jetson/index.html) to flash JetPack to the Jetson AGX Xavier with NVIDIA SDK Manager or other methods NVIDIA provides. Make sure the board is ready to use.

## Build kernel, dtb and D457 driver

The developers can set up the source code with NVIDIA's Jetson git repositories by using the provided setup script:

```
# Using setup script, recommended for developers. If JetPack version is not given, default version will be chosen.
./setup_workspace.sh [JetPack_version]
```

Or download Jetson Linux source code tarball from 
- [JetPack 6.0-DP BSP sources](https://developer.nvidia.com/downloads/embedded/l4t/r36_release_v2.0/sources/public_sources.tbz2)
- [JetPack 5.1.2 BSP sources](https://developer.nvidia.com/downloads/embedded/l4t/r35_release_v4.1/sources/public_sources.tbz2) 
- [JetPack 5.0.2 BSP sources](https://developer.nvidia.com/embedded/l4t/r35_release_v1.0/sources/public_sources.tbz2) 
- [JetPack 4.6.1 BSP sources](https://developer.nvidia.com/embedded/l4t/r32_release_v7.1/sources/t186/public_sources.tbz2)

```
# JetPack 6.0
mkdir -p l4t-gcc/6.0
cd ./l4t-gcc/6.0
wget https://developer.nvidia.com/downloads/embedded/l4t/r36_release_v2.0/toolchain/aarch64--glibc--stable-2022.08-1.tar.bz2 -O aarch64--glibc--stable-final.tar.bz2
tar xf aarch64--glibc--stable-final.tar.bz2 --strip-components 1
cd ../..
wget https://developer.nvidia.com/downloads/embedded/l4t/r36_release_v2.0/sources/public_sources.tbz2
tar xjf public_sources.tbz2
cd Linux_for_Tegra/source
tar xjf kernel_src.tbz2
tar xjf kernel_oot_modules_src.tbz2
tar xjf nvidia_kernel_display_driver_source.tbz2

# JetPack 5.1.2
mkdir -p l4t-gcc/5.1.2
cd ./l4t-gcc/5.1.2
wget https://developer.nvidia.com/embedded/jetson-linux/bootlin-toolchain-gcc-93 -O aarch64--glibc--stable-final.tar.gz
tar xf aarch64--glibc--stable-final.tar.gz
cd ../..
wget https://developer.nvidia.com/downloads/embedded/l4t/r35_release_v4.1/sources/public_sources.tbz2
tar xjf public_sources.tbz2
cd Linux_for_Tegra/source/public
tar xjf kernel_src.tbz2

# JetPack 5.0.2
mkdir -p l4t-gcc/5.0.2
cd ./l4t-gcc/5.0.2
wget https://developer.nvidia.com/embedded/jetson-linux/bootlin-toolchain-gcc-93 -O aarch64--glibc--stable-final.tar.gz
tar xf aarch64--glibc--stable-final.tar.gz --strip-components 1
cd ../..
wget https://developer.nvidia.com/embedded/l4t/r35_release_v1.0/sources/public_sources.tbz2
tar xjf public_sources.tbz2
cd Linux_for_Tegra/source/public
tar xjf kernel_src.tbz2

# JetPack 4.6.1
mkdir -p l4t-gcc/4.6.1
cd ./l4t-gcc/4.6.1
wget http://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/aarch64-linux-gnu/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu.tar.xz
tar xf gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu.tar.xz --strip-components 1
cd ../..
wget https://developer.nvidia.com/embedded/l4t/r32_release_v7.1/sources/t186/public_sources.tbz2
tar xjf public_sources.tbz2
cd Linux_for_Tegra/source/public
tar xjf kernel_src.tbz2
```

Apply D457 patches and build the kernel image, dtb and D457 driver.

```
# if using setup script
./apply_patches.sh [--one-cam | --dual-cam] apply [JetPack_version]

# or, if using direct download method
# ./apply_patches_ext.sh [--one-cam | --dual-cam] ./Linux_for_tegra/source/public [JetPack_version]
# for JP6.0, source path is ./Linux_for_tegra/source :
# ./apply_patches_ext.sh ./Linux_for_tegra/source 6.0

Note: The `--one-cam` and `--dual-cam` option applies only for JetPack 5.0.2,
compatible with adapter: https://store.intelrealsense.com/buy-intel-realsense-des457.html.
By setting the `--one-cam` option it builds DT with only camera on GMSL link A (default).
By setting the `--dual-cam` option it builds DT with dual cameras on GMSL link A and B.
The default is to single camera configuration for JetPack 5.0.2.

# build kernel, dtb and D457 driver
# install dependencies
sudo apt install build-essential bc flex bison
# method 1: build kernel Debian packages
./build_all_deb.sh [--no-dbg-pkg] [JetPack_version] [JetPack_source_dir]
# method 2: build kernel and modules only
./build_all.sh [JetPack_version] [JetPack_source_dir]

# remove our patches from JetPack kernel source code if using setup script
# ./apply_patches.sh reset [JetPack_version]
```

Debian packages will be generated in `images` folder.

## Install kernel and D457 driver to Jetson Orin
<details>
<summary>JP6.0 build results</summary>

- kernel image (not modified): `images/6.0/rootfs/boot/Image`
- dtb overlay: `images/6.0/rootfs/boot/tegra234-camera-d4xx-overlay.dtbo`
- oot modules: `images/6.0/rootfs/lib/modules/5.15.122-tegra/extra`

Following steps required:

1.	Copy entire directory `images/6.0/rootfs/lib/modules/5.15.122-tegra/extra/` from host to `/lib/modules/5.15.122-tegra/extra/` on Orin
2.	Copy `tegra234-camera-d4xx-overlay.dtbo` from host to `/boot/tegra234-camera-d4xx-overlay.dtbo` on Orin
3.	Run  $ `sudo /opt/nvidia/jetson-io/jetson-io.py`
    1.	Configure Jetson AGX CSI Connector
    2.	Configure for compatible hardware
    3.	Jetson RealSense Camera D457
    4.	$ `sudo depmod`
    5.	$ `echo "d4xx" | sudo tee /etc/modules-load.d/d4xx.conf`
4.	Reboot

Copy them to the right places:

```
scp -r images/6.0/rootfs/boot/tegra234-camera-d4xx-overlay.dtbo nvidia@10.0.0.116:~/
scp -r images/6.0/rootfs/lib/modules/5.15.122-tegra/extra nvidia@10.0.0.116:~/
# RealSense metadata patched kernel modules
scp -r images/6.0/rootfs/lib/modules/5.15.122-tegra/kernel/drivers/media/v4l2-core/videodev.ko nvidia@10.0.0.116:~/
scp -r images/6.0/rootfs/lib/modules/5.15.122-tegra/kernel/drivers/media/usb/uvc/uvcvideo.ko nvidia@10.0.0.116:~/
# Kernel Image with SENSOR_HID support for RealSense USB cameras with IMU
scp -r images/6.0/rootfs/boot/Image nvidia@10.0.0.116:~/
```

on target:

```
sudo cp ~/tegra234-camera-d4xx-overlay.dtbo /boot/
# backup:
sudo tar -cjf /lib/modules/$(uname -r)/modules_$(uname -r)_extra.tar.bz2 /lib/modules/$(uname -r)/extra
sudo cp -r ~/extra /lib/modules/$(uname -r)/
# enable RealSense metadata:
sudo cp uvcvideo.ko /lib/modules/5.15.122-tegra/kernel/drivers/media/usb/uvc/uvcvideo.ko
sudo cp videodev.ko /lib/modules/5.15.122-tegra/kernel/drivers/media/v4l2-core/videodev.ko
# backup kernel (better to have additional boot entry in extlinux.conf)
sudo cp /boot/Image /boot/Image.orig
sudo cp Image /boot/Image
```

Enable d4xx overlay:

With Jetson-IO tool:
`sudo /opt/nvidia/jetson-io/jetson-io.py`

1.	Configure Jetson AGX CSI Connector
2.	Configure for compatible hardware
3.	Jetson RealSense Camera D457

With command line

`sudo /opt/nvidia/jetson-io/config-by-hardware.py -n 2="Jetson RealSense Camera D457"`

Expected:
```
nvidia@ubuntu:~$ sudo /opt/nvidia/jetson-io/config-by-hardware.py -n 2="Jetson RealSense Camera D457"
Configuration saved to /boot/tegra234-camera-d4xx-overlay.dtbo.
Reboot system to reconfigure.
```
Enable d4xx autoload:

`echo "d4xx" | sudo tee /etc/modules-load.d/d4xx.conf`

`sudo depmod`

---

</details>

## Install kernel and D457 driver to Jetson AGX Xavier

1. Install the kernel and modules

1.1 If building with `build_all_deb.sh`

Copy the Debian package `linux-image-5.10.104-d457_5.10.104-d457-1_arm64.deb` to the Jetson AGX Xavier board and install with `sudo dpkg -i linux-image-5.10.104-d457_5.10.104-d457-1_arm64.deb`. The header, libc-dev, dbg and firmware packages are optional.

1.2 If building with `build_all.sh`

The necessary files are:

- kernel image `images/<JetPack_version>/arch/arm64/boot/Image`
- dtb `images/<JetPack_version>/arch/arm64/boot/dts/nvidia/tegra194-p2888-0001-p2822-0000.dtb`, or `images/4.6.1/arch/arm64/boot/dts/tegra194-p2888-0001-p2822-0000.dtb` for JetPack 4.6.1.
- D457 driver `images/<JetPack_version>/drivers/media/i2c/d4xx.ko`
- UVC Video driver `images/<JetPack_version>/drivers/media/usb/uvc/uvcvideo.ko`
- V4L2 Core Video driver `images/<JetPack_version>/drivers/media/v4l2-core/videobuf-core.ko`
- V4L2 VMalloc Video driver `images/<JetPack_version>/drivers/media/v4l2-core/videobuf-vmalloc.ko`

And copy them to the right places:
```
sudo cp Image /boot
sudo cp tegra194-p2888-0001-p2822-0000.dtb /boot/dtb
sudo cp d4xx.ko /lib/modules/5.10.104-tegra/kernel/drivers/media/i2c/
sudo cp uvcvideo.ko /lib/modules/5.10.104-tegra/kernel/drivers/media/usb/uvc/
sudo cp videobuf-core.ko /lib/modules/5.10.104-tegra/kernel/drivers/media/v4l2-core/
sudo cp videobuf-vmalloc.ko /lib/modules/5.10.104-tegra/kernel/drivers/media/v4l2-core/
sudo depmod
```

2. Edit `/boot/extlinux/extlinux.conf` primary boot option's LINUX/FDT lines to use built kernel image and dtb file:

```
LINUX /boot/Image-5.10.104-d457
FDT /usr/lib/linux-image-5.10.104-d457/tegra194-p2888-0001-p2822-0000.dtb
```

3. Make D457 I2C module autoload at boot time: `echo "d4xx" | sudo tee /etc/modules-load.d/d4xx.conf`

After rebooting Jetson, the D457 driver should work.

**NOTE**

- Each JetPack version's kernel may be different, the user needs to change the kernel version in file names and paths accordingly, for example for JetPack 4.6.1 the version is `4.9.253-d457` or `4.9.253-tegra`, depending on the build method applied.
- For JetPack 4.6.1, the dtb file is not included in the deb package. User needs to manually copy `images/4.6.1/arch/arm64/boot/dts/tegra194-p2888-0001-p2822-0000.dtb` file to board and edit `extlinux.conf` to point to it.
- It's recommended to save the original kernel image as backup boot option in `/boot/extlinux/extlinux.conf`.