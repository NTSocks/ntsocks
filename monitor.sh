#!/bin/bash

if [ $# != 1 ]; then
echo "Usage: $0 [NTSocks Base Path]"
echo " e.g.: $0 ~/NTSock"
exit 1;
fi

BASE_DIR=$1

monitor_pid=`pidof ntb-monitor`
if [ $monitor_pid ]
then
        echo "kill monitor-pid:$monitor_pid"
        `pkill ntb-monitor`
else
        echo "ntb-monitor not running"
fi

cd $BASE_DIR
cd ./monitor/bin/
./ntb-monitor -c ~/ntm.cfg
