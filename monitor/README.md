
# ntb-monitor component

Ntb-monitor (ntm) is located at control plane and responsible for managing global ntsocks resources (e.g., port, socket), ntb connection state, establishing connection, shutting down connection, forwarding new connection requests, and instructing ntb-proxy (ntp) component to /init/create ntb connections in data plane.


## Code lines statistics for ntb-monitor

C/C++ code lines: **5131 lines**.

```
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                               23           1189            917           4005
C/C++ Header                    26            574            843           1126
-------------------------------------------------------------------------------
SUM:                            49           1763           1760           5131
-------------------------------------------------------------------------------
```



## Quick Start
**Note: Before running ntb-monitor, ntb-proxy component should be executed in back-end manner.**
Build -> Execute.

1. Build ntm
You must confirm whether the config file (**ntm.cfg**) exists or not in current directory. Or, you can specify the customized config file path with option `./bin/ntb-monitor -c [config file path]`.

    - `cd ./monitor`
    - `make clean`
    - `make`


2. Execute ntm
You can use `./bin/ntb-monitor -h` to review the detailed description about the usage of `./bin/ntb-monitor` command.

    - `./bin/ntb-monitor -h`
        ```sh
        [spark2@supervisor-1 monitor]$ ./bin/ntb-monitor -h
        ./bin/ntb-monitor: option requires an argument -- 'h'
        Usage:
        ./bin/ntb-monitor            start a server and wait for connection
        ./bin/ntb-monitor <host>     connect to server at <host>

        Options:
        -h, --help        	  	print the help information
        -s, --server 			listen ip addr
        -p, --port 			 listen on/connect to port
        -c, --conf			 monitor config file path
        ```

    - Update **listen_ip** option in `ntm.cfg` file with the IP address of current host machine.

    - `./bin/ntb-monitor`

        ```
        [spark2@supervisor-1 monitor]$ ./bin/ntb-monitor 
        Mar 10 2020 15:43:10[DEBUG] [src/ntb_monitor.c:ntm_init:181]: load the ntm config file
        Mar 10 2020 15:42:45[DEBUG] [src/config.c:load_conf:52]: load ntb monitor configuration from conf file (./ntm.cfg)
        Mar 10 2020 15:42:45[DEBUG] [src/config.c:load_conf:110]: ntb monitor configuration listen_ip=10.10.88.202
        Mar 10 2020 15:42:45[DEBUG] [src/config.c:load_conf:110]: ntb monitor configuration listen_port=9090
        Mar 10 2020 15:42:45[DEBUG] [src/config.c:load_conf:110]: ntb monitor configuration max_concurrency=128
        Mar 10 2020 15:42:45[DEBUG] [src/config.c:load_conf:110]: ntb monitor configuration max_port=1024
        Mar 10 2020 15:42:45[DEBUG] [src/config.c:load_conf:110]: ntb monitor configuration ntsock_max_conn_num=1024
        Mar 10 2020 15:43:10[DEBUG] [src/ntb_monitor.c:ntm_init:257]: Init global socket and port resources
        Mar 10 2020 15:42:45[DEBUG] [src/ntp_ntm_shm.c:ntp_ntm_shm:28]: [ntp_ntm_shm] create shm_ctx pass
        Mar 10 2020 15:43:10[DEBUG] [src/ntp_ntm_shmring.c:ntp_ntm_get_shmring:155]: ntp_ntm get shmring start
        shm_open: No such file or directory
        Mar 10 2020 15:42:45[DEBUG] [src/ntp_ntm_shm.c:ntp_ntm_shm_connect:61]: ntp_ntm_shm_connect pass
        Mar 10 2020 15:42:44[DEBUG] [src/ntm_ntp_shm.c:ntm_ntp_shm:27]: [ntm_ntp_shm] create shm_ctx pass
        Mar 10 2020 15:42:45[DEBUG] [src/ntm_ntp_shmring.c:ntm_ntp_get_shmring:155]: ntm_ntp get shmring start
        shm_open: No such file or directory
        Mar 10 2020 15:42:44[DEBUG] [src/ntm_ntp_shm.c:ntm_ntp_shm_connect:60]: ntm_ntp_shm_connect pass
        Mar 10 2020 15:43:10[DEBUG] [src/ntm_shm.c:ntm_shm:17]: create shm_ctx pass
        Mar 10 2020 15:43:10[DEBUG] [src/ntm_shm.c:ntm_shm_accept:28]: get shm_ctx pass 
        Mar 10 2020 15:42:45[DEBUG] [src/ntm_shmring.c:ntm_shmring_init:61]: init ntm_shmring with shm_addr=/ntm-shm-ring
        Mar 10 2020 15:42:45[DEBUG] [src/ntm_shmring.c:ntm_shmring_init:71]: sem_open mutex_sem pass
        Mar 10 2020 15:42:45[DEBUG] [src/ntm_shmring.c:ntm_shmring_init:87]: shm_open pass
        Mar 10 2020 15:42:45[DEBUG] [src/ntm_shmring.c:ntm_shmring_init:95]: ftruncate pass
        Mar 10 2020 15:42:45[DEBUG] [src/ntm_shmring.c:ntm_shmring_init:108]: mmap pass
        Mar 10 2020 15:42:45[DEBUG] [src/ntm_shmring.c:ntm_shmring_init:117]: sem_open buf_count_sem pass
        Mar 10 2020 15:42:45[DEBUG] [src/ntm_shmring.c:ntm_shmring_init:125]: sem_open spool_signal_sem pass
        Mar 10 2020 15:42:45[DEBUG] [src/ntm_shmring.c:ntm_shmring_init:136]: ntm shmring init successfully!
        Mar 10 2020 15:43:10[DEBUG] [src/ntm_shm.c:ntm_shm_accept:38]: ntm_shm_accept pass
        Mar 10 2020 15:43:10[DEBUG] [src/ntb_monitor.c:ntm_init:344]: ntm_init success
        Mar 10 2020 15:43:10[DEBUG] [src/ntb_monitor.c:nts_shm_recv_thread:2069]: nts_recv_thread ready...
        Mar 10 2020 15:43:10[DEBUG] [src/ntb_monitor.c:ntm_sock_listen_thread:1130]: local ntb-monitor ready to receive the connection request from remote ntb-monitor.
        Mar 10 2020 15:43:10[DEBUG] [src/ntb_monitor.c:ntm_sock_listen_thread:1141]: ntm listen socket start listening...
        Mar 10 2020 15:43:10[DEBUG] [src/ntb_monitor.c:ntm_sock_listen_thread:1145]: ready to accept connection
        ```

    - Then, you can execute **libnts** (a general and easy-to-use socket-like ntsocks library) based server/client applications.



## Deploy and Benchmark

scp bin/ntb-monitor ntsocks@10.10.88.210:/home/ntsocks/ntsocks-deploy

scp bin/ntb-monitor ntsocks@10.10.88.211:/home/ntsocks/ntsocks-deploy

scp src/lib/.libs/libnts.so* ntsocks@10.10.88.210:/home/ntsocks/ntsocks-deploy/

scp src/lib/.libs/libnts.so* ntsocks@10.10.88.211:/home/ntsocks/ntsocks-deploy/

scp -r examples/ src/ ntsocks@10.10.88.210:/home/ntsocks/ntsocks-deploy/libnts/build

scp -r examples/ ntsocks@10.10.88.210:/home/ntsocks/ntsocks-deploy/libnts/build

scp -r examples/ src/ ntsocks@10.10.88.211:/home/ntsocks/ntsocks-deploy/libnts/build

scp -r examples/ ntsocks@10.10.88.211:/home/ntsocks/ntsocks-deploy/libnts/build

LD_PRELOAD=./libnts.so ./examples/hello-ntsock
LD_PRELOAD=./libnts.so ./examples/client-ntsock

sudo su 切换到root权限执行 ntb-monitors



