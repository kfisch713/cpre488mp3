#!/bin/ash

echo "1-1:1.0" > /sys/bus/usb/drivers/usbhid/unbind
insmod /mnt/launcher_driver.ko
