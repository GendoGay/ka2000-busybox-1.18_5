#!/bin/sh
###### Busybox build #############
export PATH=/home/arm-2010q1/bin:$PATH
make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- -f Makefile.micro
#make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- install
#arm-none-linux-gnueabi-readelf -a _install/bin/busybox
