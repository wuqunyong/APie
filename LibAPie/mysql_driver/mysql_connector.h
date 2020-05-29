#pragma once

#ifdef WIN32
#include "../network/windows_platform.h"
#else
#include <unistd.h>
#include <pthread.h>
#endif

#include <string>
#include <queue>
#include <sstream>
#include <stdlib.h>

#include <mysql.h>

#include "result_set.h"
#include "mysql_table.h"
#include "mysql_field.h"

struct MySQLConnectOptions
{
	MySQLConnectOptions()
	{
		port = 0;
	}

	std::string host;
	std::string user;
	std::string passwd;
	std::string db;
	unsigned int port;
};

class MySQLConnector
{
public:
	MySQLConnector();
	~MySQLConnector();

public:
	void init(MySQLConnectOptions& options);
	bool connect(void);
	bool query(const char *q, unsigned long length, ResultSet* &results, bool flags = true);
	bool executeSQL(const char *q, unsigned long length);
	void close(void);

	bool describeTable(const std::string tableName, MysqlTable& table);

private:
	void initData();

public:

	std::string getError();
    bool escapeString(const std::string& from, std::string& to);

	uint32_t getLastError(void)
	{
		return mysql_errno(this->mysql_);
	}

	MYSQL* getHandle(void)
	{
		return this->mysql_;
	}

	my_ulonglong getAffectedRows()
	{
		return this->affected_rows_;
	}

	my_ulonglong getInsertId()
	{
		return this->insert_id_;
	}

private:
	bool handleMySQLErrno(uint32_t err_no);
	bool canTryReconnect(void);
	bool dumpTableStructure(MYSQL_RES* pRES, MysqlTable& table);

private:
	MYSQL* mysql_;
	MySQLConnectOptions  options_;
	uint32_t  re_connect_count_;

	my_ulonglong affected_rows_;
	my_ulonglong insert_id_;

	std::string error_;

	//(my_ulonglong)-1
};
