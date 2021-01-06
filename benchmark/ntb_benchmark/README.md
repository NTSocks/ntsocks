## Quick Start

1. make clean

2. make all

##### Use -help for parameter usage

```sh
./bin/server -help
```

##### or
```sh
./bin/client -help
```

### execute code sample
##### server side
```sh
LD_PRELOAD=/home/ntsocks/ntsocks-deploy/libnts.so ./bin/server -w -s 64 -t 4
```
##### client side
```sh
LD_PRELOAD=/home/ntsocks/ntsocks-deploy/libnts.so ./bin/client -w -s 64 -t 4
```

### Use `benchmark_autorun_to_file.sh` for complete test auto-running

```sh
sudo vim /etc/ntp.cfg
# Do: change the num_partition (partition, -r) and data_packet_size (packet size, -c)
vim nts.cfg
# Do: change the mtu_size (packet size, -c)
vim benchmark_autorun_to_file.sh
# Do: change the args of auto run

./benchmark_autorun_to_file.sh N # NTSocks benchmark auto run
./benchmark_autorun_to_file.sh T # TCP benchmark auto run
```



### Use `benchmark_autorun_to_file.sh` for specific running

```sh
sudo vim /etc/ntp.cfg
# Do: change the num_partition (partition, -r) and data_packet_size (packet size, -c)
vim nts.cfg
# Do: change the mtu_size (packet size, -c)

./benchmark_autorun.sh SN <metric> <payload_size> <req_num> <thread> <partition_num> <packet_size> # NTSocks benchmark specific run
./benchmark_autorun.sh ST <metric> <payload_size> <req_num> <thread> # TCP benchmark specific run
```

### Use `auto_data_export.py` for data collation

```sh
cd [data_directory]
./auto_data_export.py
```