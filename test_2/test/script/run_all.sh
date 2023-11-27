#!/bin/bash

python_path=/home/osm/test_2/test/python
output_path=/home/osm/test_2/test/output

sudo /usr/src/linux-ksm/dsa-setup.sh -d dsa2 -w 1 -m d
insmod /home/osm/test_2/dsa_module/dsatest_osm.ko

# DSA MEMCOMP ONLY
rm $output_path/output_dsa
echo 1 > /sys/module/dsatest_osm/parameters/dsatest_dsa_on
echo 1 > /sys/module/dsatest_osm/parameters/dsatest_run
sleep 1

pqos -o $output_path/output_dsa -t 60 -mllc:0-3
echo 0 > /sys/module/dsatest_osm/parameters/dsatest_run
sleep 1

# SW MEMCOMP ONLY
rm $output_path/output_sw
echo 0 > /sys/module/dsatest_osm/parameters/dsatest_dsa_on
echo 1 > /sys/module/dsatest_osm/parameters/dsatest_run
sleep 1

pqos -o $output_path/output_sw -t 60 -mllc:0-3
echo 0 > /sys/module/dsatest_osm/parameters/dsatest_run
sleep 1

rmmod dsatest_osm

# Make Graph
for type in sw dsa
do
    python3 $python_path/make_csv.py output_$type output_$type.csv
    python $python_path/make_graph.py output_$type.csv &
done
