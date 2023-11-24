#!/bin/bash

virsh vcpupin Server 0 0
virsh vcpuinfo Server
virsh vcpupin Client_1 0 1
virsh vcpuinfo Client_1
virsh vcpupin Client_2 0 2
virsh vcpuinfo Client_2
virsh vcpupin Client_3 0 3
virsh vcpuinfo Client_3

for ((i=4; i<80; i++));
do
    echo 0 > /sys/devices/system/cpu/cpu${i}/online
done

chmem -bd 0-128
#sudo cpupower frequency-set --governor userspace
#sudo cpupower frequency-set --freq 2100000
