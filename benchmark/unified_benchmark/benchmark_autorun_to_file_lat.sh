#!/bin/bash

# 1. Set args
NTS_ROOTPATH_210=$(cd ../..; pwd)
NTS_ROOTPATH_211=${NTS_ROOTPATH_210/ntb-server1/ntb-server2}
NTS_RUNNINGPATH_210=$NTS_ROOTPATH_210/benchmark/unified_benchmark/
NTS_RUNNINGPATH_211=${NTS_RUNNINGPATH_210/ntb-server1/ntb-server2}
NTS_FILE_PATH=$NTS_RUNNINGPATH_210/NTS_data/LAT/

metric=0        # 0: Latency; 1: Bandwidth; 2: Throughput (-l)
header_size=8   # Header size
port=8099       # Origin Port Number, Increase by 1 after each run (-p)
requests=100000 # Request number (-n)


# 2. Clean NTSocks Environment
$NTS_ROOTPATH_210/nts_down.sh


# 3. Run NTSocks benchmark
# Parse arguments
partition_arr=(1)                                                    # Partition (/etc/ntp.cfg sublink_num) (-r)
packet_size_arr=(1024 2048 4096 8192)                                # Packet size (/etc/ntp.cfg & nts.cfg) (-c)
payload_size_arr=(8 16 32 64 128 256 512 1024 2048 4096 8192)        # Payload Size (-s), header size is 8 bytes
origin_payload_size_arr=(8 16 32 64 128 256 512 1024 2048 4096 8192) # Payload Size (-s), header size is 8 bytes
thread_arr=(1)                                                       # Thread Numbers (-t)

for partition in "${partition_arr[@]}"; do

    for packet_size in "${packet_size_arr[@]}"; do

        # 3.1 Change nts.cfg and /etc/ntp.cfg
        cd $NTS_RUNNINGPATH_210
        cp nts_cfg/nts_$((${packet_size} / 1000))k.cfg nts.cfg
        cat nts.cfg
        sudo cp ntp_cfg/ntp_${partition}_$((${packet_size} / 1000))k.cfg /etc/ntp.cfg
        cat /etc/ntp.cfg

        ssh ntb-server2@10.176.22.211 <<eeooff
cd $NTS_RUNNINGPATH_211
cp nts_cfg/nts_$((${packet_size} / 1000))k.cfg nts.cfg
sudo cp ntp_cfg/ntp_${partition}_$((${packet_size} / 1000))k.cfg /etc/ntp.cfg
exit
eeooff

        # 3.2 Adjust the size of payload_size_arr
        payload_size_arr=("${origin_payload_size_arr[@]}")
        i=0
        for data_size in "${payload_size_arr[@]}"; do
            if [[ $data_size -ge $packet_size ]]; then
                payload_size_arr[i]=$((${payload_size_arr[$i]} - $(($data_size / $packet_size)) * $header_size))
            fi
            let i++
        done

        for payload_size in "${payload_size_arr[@]}"; do

            for thread in "${thread_arr[@]}"; do
                echo "payload size: $payload_size, partition: $partition, packet_size: $packet_size, thread: $thread"

                pnum=$(lsof -i:$port | wc -l)
                while [[ $pnum -ne 0 ]]; do
                    let port++
                    pnum=$(lsof -i:$port | wc -l)
                done

                # 3.3 Start 210 Proxy
                echo "========== Starting PROXY 210 =========="
                $NTS_ROOTPATH_210/proxy.sh $NTS_ROOTPATH_210 > ntp.log &

                # 3.4 Start 211 Proxy
                echo "========== Starting PROXY 211 =========="
                ssh ntb-server2@10.176.22.211 <<eeooff
nohup $NTS_ROOTPATH_211/proxy.sh $NTS_ROOTPATH_211 > ntp.log &
sleep 3
exit
eeooff

                # 3.5 Start 210 Monitor
                echo "========== Starting MONITOR 210 =========="
                $NTS_ROOTPATH_210/monitor.sh $NTS_ROOTPATH_210 >ntm.log &

                # 3.6 Start 211 Monitor
                echo "========== Starting MONITOR 211 =========="
                ssh ntb-server2@10.176.22.211 <<eeooff
nohup $NTS_ROOTPATH_211/monitor.sh $NTS_ROOTPATH_211 > ntm.log &
sleep 1
exit
eeooff

                # 3.7 Run 210 (Server)
                echo "========== Running 210 NTS Application =========="
                cd $NTS_RUNNINGPATH_210
                LD_PRELOAD=$NTS_ROOTPATH_210/libnts/build/src/lib/.libs/libnts.so ./bin/server -p $port -l $metric -s $payload_size -n $requests -t $thread -r $partition -c $packet_size -m 2 -w -f $NTS_FILE_PATH &
                sleep 1

                # 3.8 Run 211 (Client)
                echo "========== Running 211 NTS Application =========="
                ssh ntb-server2@10.176.22.211 <<eeooff
cd $NTS_RUNNINGPATH_211
LD_PRELOAD=$NTS_ROOTPATH_211/libnts/build/src/lib/.libs/libnts.so ./bin/client -p $port -l $metric -s $payload_size -n $requests -t $thread -r $partition -c $packet_size -m 2 -w &
exit
eeooff

                echo "********** [END] Partition = $partition, Packet Size = $packet_size, Payload Size = $payload_size, # of thread = $thread **********"
                sleep 1 # Must wait for fclose() to flushing all data to file

                # 3.9 Clean NTS environment for next loop
                $NTS_ROOTPATH_210/nts_down.sh
            done
        done
    done
done


# 4. Clean environment
$NTS_ROOTPATH_210/nts_down.sh


echo ALL FINISHED.
