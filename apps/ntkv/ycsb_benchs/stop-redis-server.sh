#!/bin/bash
#close server on 211
ssh ntb-server2@10.176.22.211 << eeooff
pgrep redis-server
sudo pkill redis-server
echo "close redis-server"
pgrep server

eeooff