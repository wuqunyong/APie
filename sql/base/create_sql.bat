
set HOST="127.0.0.1"
set USER="root"
set PASSWORD="root"
set PORT=3306

@echo HOST %HOST%
@echo USER %USER%
@echo PASSWORD %PASSWORD%
@echo PORT %PORT%



set DATABASE="apie_account"
@echo DATABASE %DATABASE%

mysql -h%HOST% -P%PORT% -u%USER% -p%PASSWORD%  -e "DROP DATABASE IF EXISTS apie_account;CREATE DATABASE IF NOT EXISTS apie_account DEFAULT CHARACTER SET utf8;"
mysql -h%HOST% -P%PORT% -u%USER% -p%PASSWORD% %DATABASE% < .\apie_account.sql


set DATABASE="apie"
@echo DATABASE %DATABASE%

mysql -h%HOST% -P%PORT% -u%USER% -p%PASSWORD%  -e "DROP DATABASE IF EXISTS apie;CREATE DATABASE IF NOT EXISTS apie DEFAULT CHARACTER SET utf8;"
mysql -h%HOST% -P%PORT% -u%USER% -p%PASSWORD% %DATABASE% < .\apie.sql

pause

rem use mysql
rem alter user 'root'@'localhost' identified with mysql_native_password by '123456';
rem flush privileges;

rem grant all PRIVILEGES on apie_account.* to 'root'@'%' identified by '123456' WITH GRANT OPTION;
rem flush privileges;