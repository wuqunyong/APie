#!/bin/bash

PROG="poseidon_mgr"
logFile=/usr/local/poseidon-mgr/logs/monitor/monitor

if [ ! -e "/usr/local/poseidon-mgr/logs/monitor" ]; then
	/bin/mkdir -p /usr/local/poseidon-mgr/logs/monitor
fi

#1. Check poseidon_mgr existence.
if [ ! -f "/usr/local/poseidon-mgr/bin/poseidon_mgr" ]; then
	/bin/echo "`date +'%Y-%m-%d %H:%M:%S'`| poseidon_mgr not installed" >> ${logFile}
	exit 1
fi

#2. Check poseidon_mgr running 
#/etc/init.d/poseidon_mgr status

pid=`ps -ef | grep -v grep | grep /usr/local/poseidon-mgr/bin/poseidon_mgr | awk '{print $8 "|" $2}'`;
count=`ps -ef | grep -v grep | grep /usr/local/poseidon-mgr/bin/poseidon_mgr | wc -l`;
if [ $count -lt 1 ]
then

	if [ ! -f "/usr/local/poseidon-mgr/conf/MASTER" ]; then
		/bin/echo "`date +'%Y-%m-%d %H:%M:%S'`| poseidon_mgr backup exit" >> ${logFile}
		exit 1
	fi

	ip=`/sbin/ifconfig -a|grep inet|grep -v 127.0.0.1|grep -v inet6|awk '{print $2}'|tr -d "addr:"`;
	hn=`hostname`;
	
	/bin/echo "`date +'%Y-%m-%d %H:%M:%S'`|restart before|${PROG} ${count} (pid $pid) ${ip}_${hn}" >> ${logFile}
	
	/etc/init.d/poseidon_mgr stop
	/etc/init.d/poseidon_mgr start
	/etc/init.d/poseidon_mgr status

	pid=`ps -ef | grep -v grep | grep /usr/local/poseidon-mgr/bin/poseidon_mgr | awk '{print $8 "|" $2}'`;
	count=`ps -ef | grep -v grep | grep /usr/local/poseidon-mgr/bin/poseidon_mgr | wc -l`;

	/bin/echo "`date +'%Y-%m-%d %H:%M:%S'`|restart after|${PROG} ${count} (pid $pid) is running..." >> ${logFile}
	if [ $count -lt 1 ]; then
		exit 2 
	fi

	exit 0
fi

/bin/echo "`date +'%Y-%m-%d %H:%M:%S'`|running|${PROG} ${count} (pid $pid) is running..." >> ${logFile}
exit 0
