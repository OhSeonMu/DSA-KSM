#!/bin/bash
total_ip=("192.168.122.79" "192.168.122.62" "192.168.122.245" "192.168.122.209")

for ip in ${total_ip[@]}
do
    # Move YCSB Directory
    # scp -r /home/osm/YCSB caslab@${ip}:/home/caslab

    # Move Script
    scp /home/osm/test_1/vm_script/setup.sh /home/osm/script/vm_script/load.sh /home/osm/script/vm_script/run.sh caslab@${ip}:/home/caslab
done
