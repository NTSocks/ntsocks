#!/bin/bash

workload_dir=./test_workloads

for file_name in $workload_dir/R*.spec; do

    echo "$file_name"
    file=${file_name#*/}
    file=${file#*/}
    file=${file%.*}
    echo "$file"


    nohup ./start-kv-server.sh &
    sleep 3
    echo "./ycsbc -db kvsdb -threads 1 -P $file_name -host 192.168.2.211 -port 10000 >./result/tcp_kv/kvsdb-$file.output "
    ./ycsbc -db kvsdb -threads 1 -P $file_name -host 192.168.2.211 -port 10000 >./result/tcp_kv/kvsdb-$file.output 
    ./stop-kv-server.sh
done