#!/bin/sh
###### Busybox build #############
export PATH=/home/arm-2010q1/bin:$PATH
#make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- clean 
make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- menuconfig

