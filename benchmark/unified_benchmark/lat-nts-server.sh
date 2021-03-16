#!/bin/bash

prefix=`cd ../..; pwd`

LD_PRELOAD=$prefix/libnts/build/src/lib/.libs/libnts.so ./bin/server -p 9999 -t 1 -r 1 -c 1024 -m 2 -w -l 0 -s 8 -n $@
