#!/bin/bash
logFile=/usr/local/apie/logs/monitor/mysql_operator

if [ ! -e "/usr/local/apie/logs/monitor" ]; then
	/bin/mkdir -p /usr/local/apie/logs/monitor
fi

HOST="127.0.0.1"
USER="root"
PASSWORD="root"
PORT=3306


echo HOST $HOST
echo USER $USER
echo PASSWORD $PASSWORD
echo PORT $PORT

# apie_account
mysql -h$HOST -P$PORT -u$USER -p$PASSWORD <<EOF
DROP DATABASE IF EXISTS apie_account;
CREATE DATABASE IF NOT EXISTS apie_account DEFAULT CHARACTER SET utf8;
EOF

DATABASE="apie_account"
echo DATABASE $DATABASE


mysql -h$HOST -P$PORT -u$USER -p$PASSWORD $DATABASE < ./apie_account.sql 2>&1 | /usr/bin/tee -a ${logFile}
cat ./apie_account.sql | xargs echo "`date +'%Y-%m-%d %H:%M:%S'`|apie_account.sql|" | /usr/bin/tee -a ${logFile}

# /bin/echo "`date +'%Y-%m-%d %H:%M:%S'`|apie_account.sql " >> ${logFile}




# apie
mysql -h$HOST -P$PORT -u$USER -p$PASSWORD <<EOF
DROP DATABASE IF EXISTS apie;
CREATE DATABASE IF NOT EXISTS apie DEFAULT CHARACTER SET utf8;
EOF

DATABASE="apie"
echo DATABASE $DATABASE


mysql -h$HOST -P$PORT -u$USER -p$PASSWORD $DATABASE < ./apie.sql 2>&1 | /usr/bin/tee -a ${logFile}
cat ./apie.sql | xargs echo "`date +'%Y-%m-%d %H:%M:%S'`|apie.sql|" | /usr/bin/tee -a ${logFile}

#/bin/echo "`date +'%Y-%m-%d %H:%M:%S'`|apie.sql " >> ${logFile}


# USE apie_account;
