# !/bin/bash

file_path="/home/caslab/YCSB/workloads/workload$2"

line_number=25
new_line="recordcount=$3"
sed -i "${line_number}s/.*/${new_line}/" "$file_path"
line_number=26
new_line="operationcount=100000"
sed -i "${line_number}s/.*/${new_line}/" "$file_path"

cd /home/caslab/YCSB
./bin/ycsb load redis -s -P workloads/workloada -p "redis.host=$1" -p "redis.port=6379"
