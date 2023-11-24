#!/bin/bash

# Install VIM
sudo apt-get update -y
sudo apt-get install vim

# Install python 2.7
sudo apt-add-repository universe
sudo apt update
sudo apt install python2.7 -y
sudo update-alternatives --install /usr/bin/python python /usr/bin/python2.7 1
sudo update-alternatives --config python

# Install MVN
sudo apt-get update
sudo apt install maven -y

# Install Redis
sudo apt-get update
sudo apt-get upgrade -y
sudo apt-get install redis-server -y

# Settup Redis
file_path="/etc/redis/redis.conf"
line_number=68
new_line="bind 0.0.0.0"
sed -i "${line_number}s/.*/${new_line}/" "$file_path"
sudo systemctl restart redis-server.service

# Insatll Git 
sudo apt-get install git 
sudo apt install git

# Install YCSB
git clone https://github.com/brianfrankcooper/YCSB.git

# Settup YCSB
cd /home/caslab/YCSB
mvn -pl site.ycsb:redis-binding -am clean package
