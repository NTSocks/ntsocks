#!/bin/bash

# Parse args
parse_arg="$1"
NTS_ROOTPATH_210=/home/ntb-server1//NTSock-v2.2.1/
NTS_ROOTPATH_211=/home/ntb-server2//NTSock-v2.2.1/
s_metric="$2"
s_payload_size="$3"
s_n="$4"
s_thread="$5"
s_partition="$6"
s_packet_size="$7"
NTS_file_path=./NTSocks_data
TCP_file_path=./TCP_data

# Check if folders exist
cd $NTS_ROOTPATH_210/benchmark/ntb_benchmark/
if [ ! -d NTSocks_data ];then
    mkdir NTSocks_data
else
    echo Folder NTSocks_data Exist
fi

if [ ! -d TCP_data ];then
    mkdir TCP_data
else
    echo Folder TCP_data Exist
fi

ssh ntb-server2@10.176.22.211 << eeooff
cd $NTS_ROOTPATH_211/benchmark/ntb_benchmark/
if [ ! -d NTSocks_data ];then
    mkdir NTSocks_data
else
    echo Folder NTSocks_data Exist
fi

if [ ! -d TCP_data ];then
    mkdir TCP_data
else
    echo Folder TCP_data Exist
fi
exit
eeooff


# Benchmark Function Definition
runNTSocksBenchmark(){

# Parse arguments
# partition_arr=(1 2 4 8) # Partition (/etc/ntp.cfg sublink_num)
partition_arr=(1) # Partition (/etc/ntp.cfg sublink_num) (-r)
packet_size_arr=(128) # Packet size (/etc/ntp.cfg & nts.cfg) (-c)
payload_size_arr=(8 16 32 64 128 256 512 1024 2048 4096 8192) # Payload Size (-s)
# payload_size_arr=(128 256 512 1024 2048 4096 8192) # Payload Size (-s)
# metric_arr=(0 1) # 0 for Latency and 1 for Throughtput (-l)
metric_arr=(0) # 0 for Latency and 1 for Throughtput (-l)
thread_arr=(1 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32) # Thread Numbers (-t)
n_arr=(100000) # Request Numbers (-n)
port=8099 # Origin Port Number, Increase by 1 after each run (-p)

if [[ $parse_arg == "SN" ]] || [[ $parse_arg == "sn" ]]; then
partition_arr=("$s_partition") # Partition (-r)
packet_size_arr=("$s_packet_size") # Packet Size (-c)
payload_size_arr=("$s_payload_size") # Payload Size (-s)
metric_arr=("$s_metric") # 0 for Latency and 1 for Throughtput (-l)
thread_arr=("$s_thread") # Thread Numbers (-t)
n_arr=("$s_n") # Request Numbers (-n)
fi

for partition in "${partition_arr[@]}";
do

for packet_size in "${packet_size_arr[@]}";
do

former_partition=$(expr $partition / 2)

sudo sed -i "s/sublink_number = ${former_partition}/sublink_number = ${partition}/g" /etc/ntp.cfg
cat /etc/ntp.cfg
sleep 1

ssh ntb-server2@10.176.22.211 << eeooff
sudo sed -i "s/sublink_number = ${former_partition}/sublink_number = ${partition}/g" /etc/ntp.cfg
cat /etc/ntp.cfg
sleep 1
eeooff

for metric in "${metric_arr[@]}";
do

for payload_size in "${payload_size_arr[@]}";
do

for thread in "${thread_arr[@]}";
do

for n in "${n_arr[@]}";
do

pnum=$(lsof -i:$port | wc -l)
while [[ $pnum -ne 0 ]]
do
    let port++
    pnum=$(lsof -i:$port | wc -l)
done

echo "Partition = $partition, Metric = $metric, Payload Size = $payload_size, n = $n, # of thread = $thread, Port = $port, Partition = $partition, Packet Size = $packet_size"

# Start 210 Proxy
echo "========== Starting PROXY 210 =========="
~/proxy.sh $NTS_ROOTPATH_210 > ntp.log &

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

# Start 211 Monitor
echo "========== Starting MONITOR 211 =========="
ssh ntb-server2@10.176.22.211 << eeooff
nohup ~/monitor.sh $NTS_ROOTPATH_211 > ntm.log &
sleep 1
exit
eeooff

# Run 210 (Server)
echo "========== Running 210 NTS Application =========="
cd $NTS_ROOTPATH_210/benchmark/ntb_benchmark/
LD_PRELOAD=$NTS_ROOTPATH_210/libnts/build/src/lib/.libs/libnts.so ./bin/server -p $port -l $metric -s $payload_size -n $n -t $thread -r $partition -c $packet_size -w -f $NTS_file_path &
sleep 1

# Run 211 (Client)
echo "========== Running 211 NTS Application =========="
ssh ntb-server2@10.176.22.211 << eeooff
cd $NTS_ROOTPATH_211/benchmark/ntb_benchmark/
LD_PRELOAD=$NTS_ROOTPATH_211/libnts/build/src/lib/.libs/libnts.so ./bin/client -p $port -l $metric -s $payload_size -n $n -t $thread -r $partition -c $packet_size -w &
exit
eeooff

# Kill 210 Server, monitor, proxy
echo "========== Start Killing 210 NTSocks Process =========="
if pgrep ntb-monitor; then pkill ntb-monitor; fi
if pgrep ntb_proxy; then sudo pkill ntb_proxy; fi

# Kill 211 Client, monitor, proxy
ssh ntb-server2@10.176.22.211 << eeooff
echo "========== Start Killing 211 NTSocks Process =========="
if pgrep ntb-monitor; then pkill ntb-monitor; fi
if pgrep ntb_proxy; then sudo pkill ntb_proxy; fi
exit
eeooff

echo "********** [END] Partition = $partition, Metric = $metric, Payload Size = $payload_size, n = $n, # of thread = $thread, Port = $port, Partition = $partition, Packet Size = $packet_size **********"

done
done
done
done
done
done

}


runTCPSocketBenchmark(){

# Parse arguments
payload_size_arr=(8 16 32 64 128 256 512 1024 2048 4096 8192) # Payload Size (-s)
metric_arr=(0 1) # 0 for Latency and 1 for Throughtput (-l)
thread_arr=(1 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32) # Thread Numbers (-t)
n_arr=(100000) # Request Numbers (-n)
port=9099 # Origin Port Number, Increase by 1 after each run (-p)

if [[ $parse_arg == "ST" ]] || [[ $parse_arg == "st" ]]; then
payload_size_arr=("$s_payload_size") # Payload Size (-s)
metric_arr=("$s_metric") # 0 for Latency and 1 for Throughtput (-l)
thread_arr=("$s_thread") # Thread Numbers (-t)
n_arr=("$s_n") # Request Numbers (-n)
fi

for metric in "${metric_arr[@]}";
do

for payload_size in "${payload_size_arr[@]}";
do

for thread in "${thread_arr[@]}";
do

for n in "${n_arr[@]}";
do

pnum=$(lsof -i:$port | wc -l)
while [[ $pnum -ne 0 ]]
do
    let port++
    pnum=$(lsof -i:$port | wc -l)
done

echo "Metric = $metric, Payload Size = $payload_size, n = $n, # of thread = $thread, Port = $port"

# Run 210 (Server)
echo "========== Running 210 NTS Application =========="
cd $NTS_ROOTPATH_210/benchmark/ntb_benchmark/
./bin/server -p $port -l $metric -s $payload_size -n $n -t $thread -w -f $TCP_file_path &
sleep 1

# Run 211 (Client)
echo "========== Running 211 NTS Application =========="
ssh ntb-server2@10.176.22.211 << eeooff
cd $NTS_ROOTPATH_211/benchmark/ntb_benchmark/
./bin/client -p $port -l $metric -s $payload_size -n $n -t $thread -w &
exit
eeooff

echo "********** [END] Metric = $metric, Payload Size = $payload_size, n = $n, # of thread = $thread, Port = $port **********"

done
done
done
done

}


# NTSocks Benchmark
if [[ $parse_arg == "N" ]] || [[ $parse_arg == "n" ]] || [[ $parse_arg == "SN" ]] || [[ $parse_arg == "sn" ]]; then
echo "Executing NTSocks Benchmark.................................."
sleep 3
runNTSocksBenchmark
echo "NTSocks Benchmark Finished"

# TCP Socket Benchmark
elif [[ $parse_arg == "T" ]] || [[ $parse_arg == "t" ]] || [[ $parse_arg == "ST" ]] || [[ $parse_arg == "st" ]]; then
echo "Executing TCP Socket Benchmark.................................."
sleep 3
runTCPSocketBenchmark
echo "TCP Socket Benchmark Finished"

# Both Benchmark
elif [[ $parse_arg == "B" ]] || [[ $parse_arg == "b" ]]; then
echo "Excecuting Both Benchmark.................................."
sleep 3
runNTSocksBenchmark
runTCPSocketBenchmark
echo "Both Benchmark Finished"

# Clean data folders
elif [[ $parse_arg == "CLEAN" ]]; then
echo "Cleaning data folders.................................."
sleep 3
rm $NTS_ROOTPATH_210/benchmark/ntb_benchmark/NTSocks_data/*
rm $NTS_ROOTPATH_210/benchmark/ntb_benchmark/TCP_data/*
ssh ntb-server2@10.176.22.211 << eeooff
rm $NTS_ROOTPATH_211/benchmark/ntb_benchmark/NTSocks_data/*
rm $NTS_ROOTPATH_211/benchmark/ntb_benchmark/TCP_data/*
exit
eeooff

# Print Usage
else
    echo "========== Usage =========="
    echo "Input 'N' for NTSocks Benchmark;"
    echo "Input 'T' for TCP Socket Benchmark;"
    echo "Input 'B' for Both NTSocks and TCP Socket Benchmark;"
    echo "Input 'SN' for Specific NTSocks Benchmark Executing, Format: ./benchmark_autorun.sh SN <metric> <payload_size> <req_num> <thread> <partition_num> <packet_size>;"
    echo "Input 'ST' for Specific TCP Socket Benchmark Executing, Format: ./benchmark_autorun.sh ST <metric> <payload_size> <req_num> <thread>;"
    echo "Input 'CLEAN' for cleaning the data folders (Use with CAUTION!);"
    echo "Input 'H' for printing this usage."
fi

echo "Cleaning temp files.................................."
sleep 3
rm $NTS_ROOTPATH_210/benchmark/ntb_benchmark/temp*

if pgrep ntb-monitor; then pkill ntb-monitor; fi
if pgrep ntb_proxy; then sudo pkill ntb_proxy; fi
ssh ntb-server2@10.176.22.211 << eeooff
if pgrep ntb-monitor; then pkill ntb-monitor; fi
if pgrep ntb_proxy; then sudo pkill ntb_proxy; fi
exit
eeooff

echo "ALL FINISHED."