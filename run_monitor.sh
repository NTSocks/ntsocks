#!/bin/bash
monitor_pid=`pidof ntb-monitor`
if [ $monitor_pid ]
then
	echo "kill monitor-pid:$monitor_pid\n"
	`kill -9 $monitor_pid`
else
	echo "ntb-monitor not running\n"
fi
su ntb-server1 <<EOF
cd /home/ntsocks/ntsocks-deploy
./ntb-monitor
EOF
