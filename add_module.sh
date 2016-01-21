#!/bin/bash

#create module
make

#install module
lsmod |grep monitor_nvidia0
if [ $? -eq 0 ]
then
	echo old module exsits,delete first.
	rmmod monitor_nvidia0
fi

ls /dev|grep monitor_nvidia0
if [ $? -eq 0 ]
then
	echo old file exists,delete first.
	rm -rf /dev/monitor_nvidia0
fi

echo insmod module
insmod monitor_nvidia0.ko

echo create device file
#Get the device number,for example,250,251
major=$(cat /proc/devices|grep monitor_nvidia0|awk '{print $1}')
mknod /dev/monitor_nvidia0 c $major 0

#install show and pid programs
chmod +x show
chmod +x pid
cp show /usr/bin
cp pid /usr/bin
