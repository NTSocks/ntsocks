#!/bin/bash

NTS_ROOTPATH_210=/home/ntb-server1/NTSock
NTS_ROOTPATH_211=/home/ntb-server2/NTSock

workload_dir=./test_workloads

for file_name in $workload_dir/R*.spec; do

    echo "$file_name"
    file=${file_name#*/}
    file=${file#*/}
    file=${file%.*}
    echo "$file"

# Start 210 Proxy
echo "========== Starting PROXY 210 =========="
~/proxy.sh $NTS_ROOTPATH_210 > ntp.log &
sleep 1

# Start 211 Proxy
echo "========== Starting PROXY 211 =========="
ssh ntb-server2@10.176.22.211 << eeooff 
nohup ~/proxy.sh $NTS_ROOTPATH_211 > ntp.log &
sleep 3
exit
eeooff

# Start 210 Monitor
echo "========== Starting MONITOR 210 =========="
~/monitor.sh $NTS_ROOTPATH_210 > ntm.log &
sleep 1

# Start 211 Monitor
echo "========== Starting MONITOR 211 =========="
ssh ntb-server2@10.176.22.211 << eeooff
nohup ~/monitor.sh $NTS_ROOTPATH_211 > ntm.log &
sleep 3
exit
eeooff


    nohup ./start-nt-kv-server.sh &
    sleep 3
    echo "./ycsbc -db kvsdb -threads 1 -P $file_name -host 10.176.22.211 -port 10000 >./result/nt_kv/kvsdb-$file.output "
    LD_PRELOAD=$NTS_ROOTPATH_210/libnts/build/src/lib/.libs/libnts.so ./ycsbc -db kvsdb -threads 1 -P $file_name -host 10.176.22.211 -port 10000 > ./result/nt_kv/kvsdb-$file.output 
    ./stop-kv-server.sh
    sleep 2

# Kill 210 Server, monitor, proxy
echo "========== Start Killing 210 NTSocks Process =========="
if pgrep server; then sudo pkill server; fi
if pgrep ntb-monitor; then pkill ntb-monitor; fi
if pgrep ntb_proxy; then sudo pkill ntb_proxy; fi

# Kill 211 Client, monitor, proxy
ssh ntb-server2@10.176.22.211 << eeooff
echo "========== Start Killing 211 NTSocks Process =========="
if pgrep server; then sudo pkill server; fi
if pgrep ntb-monitor; then pkill ntb-monitor; fi
if pgrep ntb_proxy; then sudo pkill ntb_proxy; fi
exit
eeooff

done
