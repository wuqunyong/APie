#!/bin/bash

PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin

PROG="keepalived"
logFile=/usr/local/poseidon-mgr/logs/monitor/keepalived_monitor

if [ ! -e "/usr/local/poseidon-mgr/logs/monitor" ]; then
	/bin/mkdir -p /usr/local/poseidon-mgr/logs/monitor
fi

#1. Check poseidon_mgr existence.
if [ ! -f "/usr/local/poseidon-mgr/bin/poseidon_mgr" ]; then
	/bin/echo "`date +'%Y-%m-%d %H:%M:%S'`| poseidon_mgr not installed" >> ${logFile}
	exit 1
fi

#2. Check keepalivedps running 
pid=`ps -ef | grep -v grep | grep /usr/sbin/keepalived | awk '{printf $8 ":" $2 ","}'`;
count=`ps -ef | grep -v grep | grep /usr/sbin/keepalived | wc -l`;
if [ $count -lt 1 ]
then

	if [ ! -e "/etc/keepalived/keepalived.conf" ]; then
		/bin/echo "`date +'%Y-%m-%d %H:%M:%S'`| keepalived not installed" >> ${logFile}
		exit 2
	fi

	findNum=`grep -c "BACKUP" /etc/keepalived/keepalived.conf`
	if [ $findNum -lt 1 ]
	then
		/bin/echo "`date +'%Y-%m-%d %H:%M:%S'`| keepalived not BACKUP" >> ${logFile}
		exit 0;
	fi

	ip=`/sbin/ifconfig -a|grep inet|grep -v 127.0.0.1|grep -v inet6|awk '{print $2}'|tr -d "addr:"`;
	hn=`hostname`;

	/bin/echo "`date +'%Y-%m-%d %H:%M:%S'`|restart before|${PROG} ${count} (pid $pid) ${ip}_${hn}" >> ${logFile}
	
	service keepalived start

	pid=`ps -ef | grep -v grep | grep /usr/sbin/keepalived | awk '{printf $8 ":" $2 ","}'`;
	count=`ps -ef | grep -v grep | grep /usr/sbin/keepalived | wc -l`;

	/bin/echo "`date +'%Y-%m-%d %H:%M:%S'`|restart after|${PROG} ${count} (pid $pid) is running..." >> ${logFile}
	if [ $count -lt 1 ]; then
		exit 3 
	fi

	exit 0
fi

/bin/echo "`date +'%Y-%m-%d %H:%M:%S'`|running|${PROG} ${count} (pid $pid) is running..." >> ${logFile}
exit 0
