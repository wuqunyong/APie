#include <cstdlib>
#include <string>
#include <iostream>

#include "MysqlDriver/MySQLConnector.h"

int main(int argc, char **argv)
{
	MySQLConnectOptions options;
	options.host = "127.0.0.1";
	options.user = "root";
	options.passwd = "root";
	options.db = "ff_base";
	options.port = 3306;

	MySQLConnector mysqlConnector;
	mysqlConnector.init(options);
	bool bResult = mysqlConnector.connect();
	if (!bResult)
	{
		return 0;
	}

	std::stringstream ss;
	ss << "DESC role_base";

	ResultSet* recordSet = NULL;
	std::string sError;
	bResult = mysqlConnector.query(ss.str().c_str(), ss.str().size(), recordSet);
	if (!bResult)
	{
		return false;
	}

	if (NULL != recordSet)
	{
		while (recordSet->MoveNext())
		{

			std::string sField;
			std::string sType;
			std::string sNull;
			std::string sKey;
			if ((*recordSet >> sField)
				&& (*recordSet >> sType)
				&& (*recordSet >> sNull)
				&& (*recordSet >> sKey))
			{
				std::cout << "sField:" << sField
					<< ",sType:" << sType
					<< ",sNull:" << sNull
					<< ",sKey:" << sKey
					<< std::endl;
			}
		}

		delete recordSet;
		recordSet = NULL;
	}
    return 0;
}
