# !/bin/bash

sudo systemctl stop redis-server -y
sudo apt-get remove --purge redis-server redis-tools -y
sudo apt-get autoremove -y
sudo apt-get update -y
sudo apt-get install redis-server -y

file_path="s/etc/redis/redis.conf"
line_number=68
new_line="binde 0.0.0.0"
sed -i "${line_number}s/.*/${new_line}/" "$file_path"

sudo systemctl restart redis-server
