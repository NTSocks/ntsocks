#!/bin/bash
rm -rf /dev/shm/
proxy_pid=`pidof ntb_proxy`
echo $proxy_pid
if [ $proxy_pid ]
then
	echo "kill proxy-pid:$proxy_pid\n"
	`kill -9 $proxy_pid`
else
	echo "ntb_proxy not running\n"
fi
su root <<EOF
cd /home/ntb-server1/NTSock/ntb-proxy/src
./build/ntb_proxy 
#> /home/ntb-server1/NTSock/ntb_proxy.log &
EOF
