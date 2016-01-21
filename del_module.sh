#!/bin/bash
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
