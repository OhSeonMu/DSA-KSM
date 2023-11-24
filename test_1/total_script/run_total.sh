#!/bin/bash
client_ip=("192.168.122.62" "192.168.122.245" "192.168.122.209")
server_ip=("192.168.122.79" "192.168.122.79" "192.168.122.79")
ksmd_pid=$(pidof ksmd)

declare -a sshpass_pids
sudo rm /home/osm/test_1/output/$1_$2_$3.dat
sudo rm /home/osm/test_1/result/$1_$2_$3_latency
echo "$1_$2_$3 Latency" >> /home/osm/test_1/result/$1_$2_$3_latency

# Set ksm
sudo /home/osm/test_1/total_script/set_ksm.sh $1

# Run YCSB
for ((i=0; i<${#client_ip[@]}; i++));
do
    echo "Run for ${client_ip[i]}"
    sudo sshpass -p 304caslab! ssh -o 'StrictHostKeyChecking=no' caslab@${client_ip[i]}  "echo 304caslab! | sudo -S /home/caslab/run.sh ${server_ip[i]}" >> /home/osm/test_1/result/$1_$2_$3_latency &
    sshpass_pids+=($!)    
done

sudo perf stat -e 'LLC-load-misses, LLC-loads, LLC-store-misses, LLC-stores' -C 0 -I 1000 --interval-count 200 -o /home/osm/test_1/output/$1_$2_$3.dat &
sudo perf stat -e 'cpu-cycles' -I 1000 --interval-count 200 -o /home/osm/test_1/output/$1_$2_$3_cpu.dat -p $ksmd_pid
wait

## Kill Perf
#for ((i=0; i<${#client_ip[@]}; i++)); 
#do
#    wait ${sshpass_pids[i]}
#    echo "stop ${sshpass_pids[i]}"
#done

#kill $perf_pid
#echo "stop perf"

