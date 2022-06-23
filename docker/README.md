# D457 MIPI on Jetson AGX Xavier
This section provides Docker Container build environment for Jetson platform
Usage
- Create build environment container and build project

	`make`

- Create build environment container

	`make env`

- Build project

	`make build`

- Clean all containers

	`make clean`


Output resources will be created in `out/Y-m-d-H-M-S/` directory
- kernel image `Image`
- dtb `tegra194-p2888-0001-p2822-0000.dtb`
- Modules `modules.tar.xz`
- D457 driver (also included in modules) `d4xx.ko`

[Install kernel and D457 driver to Jetson AGX Xavier](perc_hw_ds5u_android-jetson_tx2#install-kernel-and-d457-driver-to-jetson-agx-xavier)

Build procedure follows [Direct Download](perc_hw_ds5u_android-jetson_tx2#build-kernel-dtb-and-d457-driver) steps. 

The sources can be cached, you can put `public_sources.tbz2` to this directory
so it will be not download from 
[https://developer.nvidia.com/embedded/l4t/r32_release_v7.1/sources/t186/public_sources.tbz2](https://developer.nvidia.com/embedded/l4t/r32_release_v7.1/sources/t186/public_sources.tbz2)

## Docker Containers setup on Ubuntu

[Official guide](https://docs.docker.com/engine/install/ubuntu/)

Short steps

`sudo apt install docker.io`

`sudo usermod -aG docker $USER`

## License
This project is licensed under the [Apache License, Version 2.0](LICENSE).
Copyright 2022 Intel Corporation