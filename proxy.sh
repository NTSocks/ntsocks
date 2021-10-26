#!/bin/bash

if [ $# != 1 ]; then
echo "Usage: $0 [NTSocks Base Path]"
echo " e.g.: $0 ~/NTSock"
exit 1;
fi

BASE_DIR=$1

sudo su <<EOF
rm -rf /dev/shm/*
proxy_pid=`pidof ntb_proxy`
echo $proxy_pid
if [ $proxy_pid ]
then
        echo "kill proxy-pid:$proxy_pid"
        `sudo pkill ntb_proxy`
else
        echo "ntb_proxy not running"
fi

source /etc/profile
cd $BASE_DIR
cd ./ntb-proxy/src
./build/ntb_proxy 
EOF