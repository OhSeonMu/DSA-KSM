#!/bin/bash

if [ "$1" == "ksm" ]
then
    sudo systemctl stop ksmtuned
    sudo systemctl stop ksm
    echo 2 >/sys/kernel/mm/ksm/run
    
    sudo systemctl start ksmtuned
    sudo systemctl start ksm
    echo 0 >/sys/kernel/mm/ksm/dsa_on
    echo 1 >/sys/kernel/mm/ksm/run
    echo "ksm"
    
    grep . /sys/kernel/mm/ksm/*

elif [ "$1" == "no_ksm" ] 
then
    sudo systemctl stop ksmtuned
    sudo systemctl stop ksm
    
    echo 0 >/sys/kernel/mm/ksm/dsa_on
    echo 2 >/sys/kernel/mm/ksm/run
    echo "no_ksm"
    
    grep . /sys/kernel/mm/ksm/*

elif [ "$1" == "dsa_ksm" ] 
then
    sudo systemctl stop ksmtuned
    sudo systemctl stop ksm
    echo 2 >/sys/kernel/mm/ksm/run
    
    sudo systemctl start ksmtuned
    sudo systemctl start ksm
    sudo /usr/src/linux-ksm/dsa-setup.sh -d dsa2 -w 1 -m d
    echo 1 >/sys/kernel/mm/ksm/dsa_on
    echo 1 >/sys/kernel/mm/ksm/run
    echo "dsa_ksm"
    
    grep . /sys/kernel/mm/ksm/*
fi

