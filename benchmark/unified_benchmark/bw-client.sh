#!/bin/bash

prefix=`cd ../..; pwd`


LD_PRELOAD=$prefix/libnts/build/src/lib/.libs/libnts.so ./bin/client -p 9999 -l 1  -m 2 -d 20 -s 32 -o 65536 -t 1

#./bin/client -p 9999 -l 1  -m 2 -d 10 -s 10  -t 1


LD_PRELOAD=/home/ntb-server1/dev-repo/LAT/origin-nts/libnts/build/src/lib/.libs/libnts.so