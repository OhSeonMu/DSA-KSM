#!/bin/bash
ksmd_pid=$(pidof ksmd)

#./set_ksm.sh ksm
#sudo perf stat -e 'cpu-cycles' -I 1000 --interval-count 200 -o /home/osm/test_1/output/test_ksm_cpu.dat -p $ksmd_pid &
#sudo perf stat -e 'LLC-load-misses, LLC-loads, LLC-store-misses, LLC-stores' -C 0 -I 1000 --interval-count 200 -o /home/osm/test_1/output/test_ksm_a_0.dat
#grep . /sys/kernel/mm/ksm/* >> /home/osm/test_1/result/test_ksm_output

./set_ksm.sh dsa_ksm
#sudo perf stat -e 'cpu-cycles' -I 1000 --interval-count 200 -o /home/osm/test_1/output/test_dsa_ksm_cpu.dat -p $ksmd_pid &
sudo perf stat -e 'LLC-load-misses, LLC-loads, LLC-store-misses, LLC-stores' -C 0 -I 1000 --interval-count 200 -o /home/osm/test_1/output/test_dsa_ksm_a_0.dat
grep . /sys/kernel/mm/ksm/* >> /home/osm/test_1/result/test_dsa_ksm_output

#./set_ksm.sh no_ksm
#sudo perf stat -e 'cpu-cycles' -I 1000 --interval-count 200 -o /home/osm/test_1/output/test_ksm_cpudat -p $ksmd_pid &
#sudo perf stat -e 'LLC-load-misses, LLC-loads, LLC-store-misses, LLC-stores' -C 0 -I 1000 --interval-count 200 -o /home/osm/test_1/output/test_no_ksm_a_0.dat
#grep . /sys/kernel/mm/ksm/* >> /home/osm/test_1/result/test_no_ksm_output

for ksm_type in test_dsa_ksm
do
    echo "make $ksm_type"
    python /home/osm/test_1/python/make_csv.py $ksm_type a 0
    python /home/osm/test_1/python/make_LLC_csv.py $ksm_type a 0
    python /home/osm/test_1/python/make_LLC_p99.py $ksm_type a 0
    python /home/osm/test_1/python/make_LLC_graph.py $ksm_type a 0 &
done
