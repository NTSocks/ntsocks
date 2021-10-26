#!/bin/bash

prefix=`cd ../..; pwd`

LD_PRELOAD=$prefix/libnts/build/src/lib/.libs/libnts.so ./bin/server -p 9999 -l 1  -m 2 -d 20 -s 32760  -t 1

# 512	1024	2048	4096	8192	16384	32768	65536	131072	262144	524288	1048576	2097152	4194304	

#./bin/client -p 9999 -l 1  -m 2 -d 10 -s 10  -t 1