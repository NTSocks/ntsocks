#!/bin/bash
#close server on 211
ssh ntb-server2@10.176.22.211 << eeooff
pgrep server
sudo pkill server
echo "close server"
pgrep server

eeooff
