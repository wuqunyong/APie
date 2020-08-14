#! /bin/bash

#set -e   # Exit immediately if a simple command exits with a non-zero status.
#set -x   # activate debugging from here

OS_VER=`cat /etc/redhat-release | tr -cd "[0-9]{.}" | awk -F'.' '{print $1}'`
if [ "x$OS_VER" == "x7" ]; then
  SYSTEMCTL_SKIP_REDIRECT=1
fi

. /etc/rc.d/init.d/functions

RETVAL=0
SERVER="/usr/local/poseidon-mgr/bin/poseidon_mgr /usr/local/poseidon-mgr/conf/poseidon_mgr.ini"

# This is our service name
PROG="poseidon_mgr"
LOCKFILE=/var/lock/subsys/poseidon_mgr

start() {
  echo -n $"Starting $PROG:"

  pid=`pidof -o $$ -o $PPID -o %PPID -x ${PROG}`
  if [ -n "$pid" ]; then        
    failure
    echo
    echo $"${PROG} (pid $pid) is running..."
    return 0 
  fi

  `$SERVER` && success || failure
  RETVAL=$?
  echo
  [ $RETVAL -eq 0 ] && touch $LOCKFILE
}

stop() {
  echo -n $"Stopping $PROG: "
  # killproc $PROG -SIGTERM
  killproc $PROG -9 
  RETVAL=$?
  echo
  [ $RETVAL -eq 0 ] && rm -f $LOCKFILE
  [ $RETVAL -eq 0 ] && rm -f /var/run/${PROG}.pid
}

# See how we were called.
case "$1" in
  start)
    start
    ;;
  stop)
    stop
    ;;
  restart)
    stop
    start
    ;;
  status)
    status $PROG
    ;;
  *)
    echo $"Usage: $0 {start|stop|restart|status}"
    exit 2
esac

exit $RETVAL

