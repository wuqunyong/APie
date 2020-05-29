#pragma once

#include <stdint.h>
#include <stddef.h>

#include <string>
#include <sstream>
#include <iostream>
#include <map>

#include <mysql.h>

#include "mysql_field.h"

class MysqlTable
{
public:
	void setDb(std::string dbName);
	void setTable(std::string tableName);
	void appendField(MysqlField field);

	std::string getDb();
	std::string getTable();
	std::vector<MysqlField>& getFields();

private:
	std::string m_db;
	std::string m_table;
	std::vector<MysqlField> m_fields;
};

