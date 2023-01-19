# D457 MIPI on Jetson AGX Xavier

## Jetson AGX Xavier board setup

Please follow the [instruction](https://docs.nvidia.com/sdk-manager/install-with-sdkm-jetson/index.html) to flash JetPack to the Jetson AGX Xavier with NVIDIA SDK Manager or other methods NVIDIA provides. Make sure the board is ready to use.

Currently Supported JetPack versions are:

- 5.0.2 (default)
- 4.6.1

## Build kernel, dtb and D457 driver

The developers can set up the source code with NVIDIA's Jetson git repositories by using the provided setup script:

```
# Using setup script, recommended for developers. If JetPack version is not given, default version will be chosen.
./setup_workspace.sh [JetPack_version]
```

Or download Jetson Linux source code tarball from [JetPack 5.0.2 BSP sources](https://developer.nvidia.com/embedded/l4t/r35_release_v1.0/sources/public_sources.tbz2), [JetPack 4.6.1 BSP sources](https://developer.nvidia.com/embedded/l4t/r32_release_v7.1/sources/t186/public_sources.tbz2).

```
# JetPack 5.0.2
mkdir -p l4t-gcc/5.0.2
cd ./l4t-gcc/5.0.2
wget https://developer.nvidia.com/embedded/jetson-linux/bootlin-toolchain-gcc-93 -O aarch64--glibc--stable-final.tar.gz
tar xf aarch64--glibc--stable-final.tar.gz
cd ../..
wget https://developer.nvidia.com/embedded/l4t/r35_release_v1.0/sources/public_sources.tbz2
tar xjf public_sources.tbz2
cd Linux_for_Tegra/source/public
tar xjf kernel_src.tbz2

# or JetPack 4.6.1
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
./apply_patches.sh apply [JetPack_version]

# or, if using direct download method
# ./apply_patches_ext.sh ./Linux_for_tegra/source/public [JetPack_version]

# build kernel, dtb and D457 driver
sudo apt install build-essential bc
./build_all.sh [JetPack_version] [JetPack_source_dir]

# remove our patches from JetPack kernel source code if using setup script
# ./apply_patches.sh reset [JetPack_version]
```

Then the necessary files are at
- kernel image `images/<JetPack_version>/arch/arm64/boot/Image`
- dtb `images/<JetPack_version>/arch/arm64/boot/dts/nvidia/tegra194-p2888-0001-p2822-0000.dtb`, or `images/4.6.1/arch/arm64/boot/dts/tegra194-p2888-0001-p2822-0000.dtb` for JetPack 4.6.1.
- D457 driver `images/<JetPack_version>/drivers/media/i2c/d4xx.ko`
- UVC Video driver `images/<JetPack_version>/drivers/media/usb/uvc/uvcvideo.ko`
- V4L2 Core Video driver `images/<JetPack_version>/drivers/media/v4l2-core/videobuf-core.ko`
- V4L2 VMalloc Video driver `images/<JetPack_version>/drivers/media/v4l2-core/videobuf-vmalloc.ko`

## Install kernel and D457 driver to Jetson AGX Xavier

Please copy the 3 files to the Jetson AGX Xavier board, and do the following:

Edit `/boot/extlinux/extlinux.conf` primary boot option's LINUX/FDT lines to use built kernel image and dtb file:

- LINUX /boot/Image
- FDT /boot/dtb/tegra194-p2888-0001-p2822-0000.dtb

FDT entry doesn't exist by default, add it.

It's recommended to save the original kernel image as backup boot option. `sudo cp /boot/Image /boot/Image.backup` and use it in the backup boot option in `/boot/extlinux/extlinux.conf`.

Then move the images in place and enable driver autoload at kernel boot.

```
sudo cp Image /boot
sudo cp tegra194-p2888-0001-p2822-0000.dtb /boot/dtb
sudo cp d4xx.ko /lib/modules/5.10.104-tegra/kernel/drivers/media/i2c/
echo "d4xx" | sudo tee /etc/modules-load.d/d4xx.conf
sudo cp uvcvideo.ko /lib/modules/5.10.104-tegra/kernel/drivers/media/usb/uvc/
sudo cp videobuf-core.ko /lib/modules/5.10.104-tegra/kernel/drivers/media/v4l2-core/
sudo cp videobuf-vmalloc.ko /lib/modules/5.10.104-tegra/kernel/drivers/media/v4l2-core/
sudo depmod
```

Please change the kernel version in path accordingly, for example for JetPack 4.6.1 the version is `4.9.253-tegra`.

After rebooting Jetson, the D457 driver should work.

## Available directives on max9295/max9296 register setting

- Dump registers
```
cat /sys/bus/i2c/drivers/max9295/30-0040/register_dump
cat /sys/bus/i2c/drivers/max9296/30-0048/register_dump
```

- Dump setting version

```
cat /sys/module/max9295/parameters/max9295_setting_verison
cat /sys/module/max9296/parameters/max9296_setting_verison
```

- Disable updating setting dynamically (updating setting manually by running script).
  **0** means disable updating setting dynamically, while **1** means enable updating setting dynamically.

```
echo 0 | sudo tee /sys/module/max9295/parameters/max9295_dynamic_update
echo 0 | sudo tee /sys/module/max9296/parameters/max9296_dynamic_update
```

- Refresh max9295/max9295 register values, this is used for forcely set serdes setting when necessary

```
echo 1 | sudo tee /sys/bus/i2c/drivers/max9295/30-0040/refresh_setting
echo 1 | sudo tee /sys/bus/i2c/drivers/max9296/30-0048/refresh_setting
```
