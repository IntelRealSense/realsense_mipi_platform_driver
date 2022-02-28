#!/bin/bash
set -e
set -x

apply_external_patches() {
cat ${PWD}/$2/* | patch -p1 --directory=${PWD}/$1/$2/
}

apply_external_patches $1 kernel/nvidia
apply_external_patches $1 kernel/kernel-4.9
apply_external_patches $1 hardware/nvidia/platform/t19x/galen/kernel-dts
