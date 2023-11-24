#!/bin/bash
total_ip=("192.168.122.79" "192.168.122.62" "192.168.122.245" "192.168.122.209")

# Setup VM
for ((i=0; i<${#total_ip[@]}; i++));
do
    echo "Settup for ${total_ip[i]}"
    sudo sshpass -p 304caslab! ssh -o 'StrictHostKeyChecking=no' caslab@${total_ip[i]}  "echo 304caslab! | sudo -S /home/caslab/setup.sh" 
done

wait
