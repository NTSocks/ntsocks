#!/bin/bash

#start server on 211
ssh ntb-server2@10.176.22.211 << eeooff
redis-server --bind 10.176.22.211 --port 10000
#exit
eeooff