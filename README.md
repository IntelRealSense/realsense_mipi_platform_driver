# D457 MIPI on Jetson AGX Xavier

## Jetson AGX Xavier board setup

Please follow the [instruction](https://docs.nvidia.com/sdk-manager/install-with-sdkm-jetson/index.html) to flash JetPack 4.6.1 to the Jetson AGX Xavier with NVIDIA SDK Manager. Currently SDK Manager requires the host to be Ubuntu 18.04 only.

Please make sure the board is ready to use.

## Build kernel, dtb and D457 driver

In previous step to flash Jetson board with NVIDIA SDK Manager, the JetPack 4.6.1 folder should already be downloaded to your home directory at `~/nvidia/nvidia_sdk/JetPack_4.6.1_Linux_JETSON_AGX_XAVIER_TARGETS/Linux_for_Tegra`. Get JetPack kernel source code using existing script in JetPack 4.6.1, or download directly from [JetPack 4.6.1 BSP sources](https://developer.nvidia.com/embedded/l4t/r32_release_v7.1/sources/t186/public_sources.tbz2).

```
# SDK Manager method, download kernel source code
cd ~/nvidia/nvidia_sdk/JetPack_4.6.1_Linux_JETSON_AGX_XAVIER_TARGETS/Linux_for_Tegra
./source_sync.sh -t tegra-l4t-r32.7.1

# direct download method
wget https://developer.nvidia.com/embedded/l4t/r32_release_v7.1/sources/t186/public_sources.tbz2
tar xjf public_sources.tbz2
cd Linux_for_Tegra/source/public
tar xjf kernel_src.tbz2
```

Apply D457 patches and build the kernel image, dtb and D457 driver.

```
# NVIDIA SDK Manager method
cd ~/nvidia/nvidia_sdk/JetPack_4.6.1_Linux_JETSON_AGX_XAVIER_TARGETS/Linux_for_Tegra/sources

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
sudo cp d4xx.ko /lib/modules/4.9.253-tegra/kernel/drivers/media/i2c/
echo "d4xx" | sudo tee /etc/modules-load.d/d4xx.conf
sudo depmod
```

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
