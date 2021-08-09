#!/bin/bash
set -e
set -x

mkdir output
g++ test_laser_power.cpp -o output/test_laser_power