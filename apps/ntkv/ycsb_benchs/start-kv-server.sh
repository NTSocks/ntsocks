#!/bin/bash

#start server on 211
ssh ntb-server2@192.168.2.211 << eeooff
cd /home/ntb-server2/repos/nt_kv
./bin/server -s 192.168.2.211 -p 10000 
#exit
eeooff