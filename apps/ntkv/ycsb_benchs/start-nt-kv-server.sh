#!/bin/bash

NTS_ROOTPATH_210=/home/ntb-server1/NTSock
NTS_ROOTPATH_211=/home/ntb-server2/NTSock

# satrt server on 211
ssh ntb-server2@10.176.22.211 << eeooff
cd /home/ntb-server2/repos/nt_kv
LD_PRELOAD=$NTS_ROOTPATH_211/libnts/build/src/lib/.libs/libnts.so ./bin/server -s 10.176.22.211 -p 10000 
echo " start server"
#exit
eeooff