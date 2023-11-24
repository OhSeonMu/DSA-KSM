#!/bin/bash
client_ip=("192.168.122.62" "192.168.122.245" "192.168.122.209")
server_ip=("192.168.122.79" "192.168.122.79" "192.168.122.79")
#server_ip=("127.0.0.1" "127.0.0.1" "127.0.0.1" )

# Load YCSB
for ((i=0; i<${#client_ip[@]}; i++));
do
    echo "Load for ${client_ip[i]}"
    sudo sshpass -p 304caslab! ssh -o 'StrictHostKeyChecking=no' caslab@${client_ip[i]}  "echo 304caslab! | sudo -S /home/caslab/load.sh ${server_ip[i]} $1 $2" & 
done

wait

