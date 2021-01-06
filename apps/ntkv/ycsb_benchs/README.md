# YCSB-C

Yahoo! Cloud Serving Benchmark in C++, a C++ version of YCSB (https://github.com/brianfrankcooper/YCSB/wiki)

## Quick Start

To build YCSB-C on Ubuntu, for example:

```
$ sudo apt-get install libtbb-dev
$ make
```

As the driver for Redis is linked by default, change the runtime library path
to include the hiredis library by:

```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```

Run Workload A with a [TBB](https://www.threadingbuildingblocks.org)-based
implementation of the database, for example:
```
./ycsbc -db tbb_rand -threads 4 -P workloads/workloada.spec
```
Note that we do not have load and run commands as the original YCSB. Specify
how many records to load by the recordcount property. Reference properties
files in the workloads dir.  The size of key is 8byte.

## Run automation script

First you should modify the ip address in start-kv-server.sh, stop-kv-server.sh, start-nt-kv-server.sh, start-redis-server.sh, stop-redis-server.sh, run_tcp_kv.sh, run_tcp_redis.sh, run_nt_kv.sh.

Then if you want to run ntSocket automation script， you should modify *NTS_ROOTPATH_210, NTS_ROOTPATH_210* to your own path in start-nt-kv-server.sh and run_nt_kv.sh. Like:

```
NTS_ROOTPATH_210=/home/ntb-server1/yukai/NTSock
NTS_ROOTPATH_211=/home/ntb-server2/yukai/NTSock
```

Use run_tcp_kv.sh to test auto-running for kv in tcp, The test case is in the ./test_workloads directory, and the output result is in the ./result/tcp_kv directory

```
./run_tcp_kv.sh
```

Use run_tcp_redis.sh to test auto-running for redis in tcp, The test case is in the ./test_workloads directory, and the output result is in the ./result/tcp_redis directory

```
./run_tcp_redis.sh
```

Use run_tcp_kv.sh to test auto-running for kv in ntsocket, The test case is in the ./test_workloads directory, and the output result is in the ./result/nt_kv directory

```
./run_nt_kv.sh
```









