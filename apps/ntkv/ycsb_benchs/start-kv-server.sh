#!/bin/bash

#start server on 211
ssh ntb-server2@10.176.22.211 << eeooff
cd /home/ntb-server2/repos/nt_kv
./bin/server -s 10.176.22.211 -p 10000 
#exit
eeooff