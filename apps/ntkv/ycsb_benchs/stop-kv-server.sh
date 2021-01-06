#!/bin/bash
#close server on 211
ssh ntb-server2@192.168.2.211 << eeooff
pgrep server
sudo pkill server
echo "close server"
pgrep server

eeooff
