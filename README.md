# D457 MIPI on Jetson AGX Xavier

## Jetson AGX Xavier board setup

Please follow the [instruction](https://docs.nvidia.com/sdk-manager/install-with-sdkm-jetson/index.html) to flash JetPack 4.6.1 to the Jetson AGX Xavier with NVIDIA SDK Manager. Currently SDK Manager requires the host to be Ubuntu 18.04 only.

Please make sure the board is ready to use.

## Build kernel, dtb and D457 driver

In previous step to flash Jetson board with NVIDIA SDK Manager, the JetPack 4.6.1 folder should already be downloaded to your home directory at `~/nvidia/nvidia_sdk/JetPack_4.6_Linux_JETSON_AGX_XAVIER_TARGETS/Linux_for_Tegra`. Get JetPack kernel source code using existing script in JetPack 4.6.1, or download directly from [JetPack 4.6 BSP sources](https://developer.nvidia.com/embedded/l4t/r32_release_v6.1/sources/t186/public_sources.tbz2).

```
# SDK Manager method, download kernel source code
cd ~/nvidia/nvidia_sdk/JetPack_4.6_Linux_JETSON_AGX_XAVIER_TARGETS/Linux_for_Tegra
./source_sync.sh -t tegra-l4t-r32.6.1

# direct download method
wget https://developer.nvidia.com/embedded/l4t/r32_release_v6.1/sources/t186/public_sources.tbz2
tar xjf public_sources.tbz2
cd Linux_for_Tegra/source/public
tar xjf kernel_src.tbz2
```

Apply D457 patches and build the kernel image, dtb and D457 driver.

```
# NVIDIA SDK Manager method
cd ~/nvidia/nvidia_sdk/JetPack_4.6_Linux_JETSON_AGX_XAVIER_TARGETS/Linux_for_Tegra/sources

# direct download method
cd Linux_for_Tegra/source/public

# get and apply our patches
git clone https://github.com/IntelRealSense/perc_hw_ds5u_android-jetson_tx2.git
cd perc_hw_ds5u_android-jetson_tx2
./apply_patches.sh apply
# or, if using direct download method
# ./apply_patches_ext.sh ..

# build kernel, dtb and D457 driver
sudo apt install gcc-aarch64-linux-gnu build-essential bc
./build_all.sh

# remove our patches from JetPack kernel source code if using NVIDIA SDK Manager method
# ./apply_patches.sh reset
```

Then the necessary files are at
- kernel image `images/arch/arm64/boot/Image`
- dtb `images/arch/arm64/boot/dts/tegra194-p2888-0001-p2822-0000.dtb`
- D457 driver `images/drivers/media/i2c/d4xx.ko`

## Boot Jetson AGX Xavier with our images

Please copy the 3 files to the Jetson AGX Xavier board, and do the following:

```
sudo cp Image /boot
sudo cp tegra194-p2888-0001-p2822-0000.dtb /boot/dtb

# edit /boot/extlinux/extlinux.conf primary boot option for the LINUX/FDT lines to use built kernel image and dtb file:
#     LINUX /boot/Image
#     FDT /boot/dtb/tegra194-p2888-0001-p2822-000.dtb // FDT entry doesn't exist by default, add it

# reboot to use built kernel and dtb
sudo reboot
```

Make sure the booted kernel is your built image. Run `dmesg | head` to check, the second line should have your built user/machine/date info:

```
[    0.000000] Linux version 4.9.253-tegra (xzhang84@flexbj-realsense) (gcc version 7.5.0 (Ubuntu/Linaro 7.5.0-3ubuntu1~18.04) ) #32 SMP PREEMPT Sun Feb 20 15:53:46 CST 2022
```

If the kernel/dtb is right, run the SerDes script and load D457 driver module on Jetson, then everything should be ready to use:

```
bash ./perc_hw_ds5u_android-jetson_tx2/scripts/SerDes_D457.sh
sudo insmod d4xx.ko
```
