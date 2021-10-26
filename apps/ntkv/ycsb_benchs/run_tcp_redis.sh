#!/bin/bash

workload_dir=./test_workloads

for file_name in $workload_dir/R*.spec; do

    echo "$file_name"
    file=${file_name#*/}
    file=${file#*/}
    file=${file%.*}
    echo "$file"

    nohup ./start-redis-server.sh &
    sleep 3
    echo "./ycsbc -db redis -threads 1 -P $file_name -host 10.176.22.211 -port 10000 -slaves 0 >./result/tcp_redis/redis-$file.output "
    ./ycsbc -db redis -threads 1 -P $file_name -host 10.176.22.211 -port 10000 -slaves 0 >./result/tcp_redis/redis-$file.output 
    ./stop-redis-server.sh
done
