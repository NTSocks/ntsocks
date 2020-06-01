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