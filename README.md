# gpu_monitor
Generate a linux kernel module, add some trace points between linux kernel and nvidia gpu driver.
It can be used to monitor all the gpu operations,include read/write/open/close timestap, and store all the records in kernel space memory.
You can use linux commands to get all the informations.

Anyway, in principle, It can monitor all the linux character devices.
