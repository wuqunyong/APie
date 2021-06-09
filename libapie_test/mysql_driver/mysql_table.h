#pragma once

#include "../network/windows_platform.h"

#include <stdint.h>
#include <stddef.h>

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <optional>

#include <mysql.h>

#include "mysql_field.h"

class MySQLConnector;

class MysqlTable
{
public:
	void setDb(std::string dbName);
	void setTable(std::string tableName);
	void appendField(MysqlField field);

	std::string getDb();
	std::string getTable();
	std::vector<MysqlField>& getFields();
	std::optional<uint32_t> getIndexByName(const std::string& name);
	std::optional<std::string> getNameByIndex(uint32_t index);

	bool generateQuerySQL(MySQLConnector& connector, const ::mysql_proxy_msg::MysqlQueryRequest& query, std::string& sql);
	bool generateInsertSQL(MySQLConnector& connector, const ::mysql_proxy_msg::MysqlInsertRequest& query, std::string& sql);
	bool generateUpdateSQL(MySQLConnector& connector, const ::mysql_proxy_msg::MysqlUpdateRequest& query, std::string& sql);
	bool generateDeleteSQL(MySQLConnector& connector, const ::mysql_proxy_msg::MysqlDeleteRequest& query, std::string& sql);
	bool generateQueryByFilterSQL(MySQLConnector& connector, const ::mysql_proxy_msg::MysqlQueryRequestByFilter& query, std::string& sql);

private:
	std::string m_db;
	std::string m_table;
	std::vector<MysqlField> m_fields;
	std::map<std::string, uint32_t> m_nameIndex;
	std::map<uint32_t, std::string> m_indexName;
};

