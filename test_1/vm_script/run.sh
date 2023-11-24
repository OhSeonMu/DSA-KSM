# !/bin/bash

cd /home/caslab/YCSB
./bin/ycsb run redis -s -P workloads/workloada -p "redis.host=$1" -p "redis.port=6379"
