#!/bin/bash

prefix=`cd ../..; pwd`


LD_PRELOAD=$prefix/libnts/build/src/lib/.libs/libnts.so ./bin/client -p 9999 -l 3  -m 2 -s 32 -d 20 -o 1024 -t 16
                                                                        
#./bin/client -p 9999 -l 1  -m 2 -d 10 -s 10  -t 1
