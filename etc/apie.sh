#! /bin/bash

#set -e   # Exit immediately if a simple command exits with a non-zero status.
#set -x   # activate debugging from here

OS_VER=`cat /etc/redhat-release | tr -cd "[0-9]{.}" | awk -F'.' '{print $1}'`
if [ "x$OS_VER" == "x7" ]; then
  SYSTEMCTL_SKIP_REDIRECT=1
fi

. /etc/rc.d/init.d/functions

RETVAL=0

SERVER[0]=/usr/local/apie/bin/db_proxy_server 
SERVER_ARGS[0]=/usr/local/apie/conf/db_account_proxy.yaml

SERVER[1]=/usr/local/apie/bin/db_proxy_server 
SERVER_ARGS[1]=/usr/local/apie/conf/db_role_proxy.yaml

SERVER[2]=/usr/local/apie/bin/gateway_server 
SERVER_ARGS[2]=/usr/local/apie/conf/gateway_server.yaml

SERVER[3]=/usr/local/apie/bin/login_server 
SERVER_ARGS[3]=/usr/local/apie/conf/login_server.yaml

SERVER[4]=/usr/local/apie/bin/route_proxy 
SERVER_ARGS[4]=/usr/local/apie/conf/route_proxy.yaml

SERVER[5]=/usr/local/apie/bin/scene_server 
SERVER_ARGS[5]=/usr/local/apie/conf/scene_server.yaml

SERVER[6]=/usr/local/apie/bin/service_registry 
SERVER_ARGS[6]=/usr/local/apie/conf/service_registry.yaml


# This is our service name
LOCKFILE=/var/lock/subsys/apie

start() {
  # pid=`pidof -o $$ -o $PPID -o %PPID -x ${PROG}`
  # if [ -n "$pid" ]; then        
  #   failure
  #   echo
  #   echo $"${PROG} (pid $pid) is running..."
  #   return 0 
  # fi

  # `$SERVER` && success || failure
  # RETVAL=$?
  # echo
  # [ $RETVAL -eq 0 ] && touch $LOCKFILE

  length=${#SERVER[@]}
  echo "length:$length"

  curValue=0
  while(( $curValue<$length ))
  do
      echo "curValue:$curValue"
      echo "start:${SERVER[$curValue]} ${SERVER_ARGS[$curValue]}"
      `${SERVER[$curValue]} ${SERVER_ARGS[$curValue]}` && success || failure
      
      RETVAL=$?
      echo "RETVAL:$RETVAL"

      let "curValue++"
  done
}

stop() {
  length=${#SERVER[@]}
  echo "length:$length"

  curValue=0
  while(( $curValue<$length ))
  do
      echo "curValue:$curValue"
      echo "killproc:${SERVER[$curValue]}"
      killproc ${SERVER[$curValue]} -9 
      RETVAL=$?
      echo "RETVAL:$RETVAL"

      let "curValue++"
  done
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
    # status $PROG
    length=${#SERVER[@]}
    echo "length:$length"

    curValue=0
    while(( $curValue<$length ))
    do
        echo "curValue:$curValue"
        echo "status:${SERVER[$curValue]}"
        status ${SERVER[$curValue]}

        let "curValue++"
    done
    ;;
  *)
    echo $"Usage: $0 {start|stop|restart|status}"
    exit 2
esac

exit $RETVAL

