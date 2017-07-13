#! /bin/bash
set -e

BOOT_PARAM=`cat packetngin-boot.param`
echo $BOOT_PARAM

sudo insmod ./drivers/dispatcher.ko
sudo modprobe msr
sudo ./pnd $BOOT_PARAM
sudo rmmod msr
sudo rmmod dispatcher
