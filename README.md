# APie

https://asmcn.icopy.site/awesome/awesome-cpp/
https://en.cppreference.com/w/cpp/links/libs


libevent编译问题
https://blog.csdn.net/guotianqing/article/details/103035642


mysql时区

https://stackoverflow.com/questions/930900/how-do-i-set-the-time-zone-of-mysql
SELECT @@global.time_zone;

SET GLOBAL time_zone = '+8:00';
SET GLOBAL time_zone = 'Europe/Helsinki';
SET @@global.time_zone = '+00:00';


SELECT @@session.time_zone;
SET @@session.time_zone = "+00:00";  
SELECT UNIX_TIMESTAMP();  
SELECT FROM_UNIXTIME(1587905440)
https://stackoverflow.com/questions/19023978/should-mysql-have-its-timezone-set-to-utc/19075291#19075291


SELECT UNIX_TIMESTAMP();
SELECT CONVERT_TZ(FROM_UNIXTIME(1587902349), @@session.time_zone, '+00:00') 


mysql时间函数
https://www.w3resource.com/mysql/date-and-time-functions/mysql-unix_timestamp-function.php


path
http://www.gameaipro.com
jps+


protobuf
chenshuo/recipes

PRI_KEY_FLAG

select TABLE_NAME, COLUMN_NAME from information_schema.COLUMNS where TABLE_SCHEMA='%s' and COLUMN_KEY='PRI' and TABLE_NAME!='_table_list' order by `TABLE_NAME`
db_name 


MINIDUMP_EXCEPTION_INFORMATION

SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
SymGetLineFromAddr

http://crashrpt.sourceforge.net/docs/html/exception_handling.html
RaiseException
AddVectoredExceptionHandler


https://nats.io/download/nats-io/nats.go/

yaml
https://github.com/jbeder/yaml-cpp/wiki/Tutorial

#include <string>
#include <codecvt>
#include <locale>

using convert_t = std::codecvt_utf8<wchar_t>;
std::wstring_convert<convert_t, wchar_t> strconverter;

std::string to_string(std::wstring wstr)
{
    return strconverter.to_bytes(wstr);
}

std::wstring to_wstring(std::string str)
{
    return strconverter.from_bytes(str);
}