#!/bin/bash

#start server on 211
ssh ntb-server2@192.168.2.211 << eeooff
redis-server --bind 192.168.2.211 --port 10000
#exit
eeooff