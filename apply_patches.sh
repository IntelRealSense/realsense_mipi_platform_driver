#!/bin/bash
set -e
set -x

apply_external_patches() {
	git -C $1 am ${PWD}/perc_hw_ds5u_android-jetson_tx2/$1/*
}

apply_external_patches kernel/nvidia
apply_external_patches kernel/kernel-4.9
apply_external_patches hardware/nvidia/platform/t19x/galen/kernel-dts