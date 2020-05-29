#include "mysql_table.h"

void MysqlTable::setDb(std::string dbName)
{
	this->m_db = dbName;
}

void MysqlTable::setTable(std::string tableName)
{
	this->m_table = tableName;
}

void MysqlTable::appendField(MysqlField field)
{
	this->m_fields.push_back(field);
}

std::string MysqlTable::getDb()
{
	return this->m_db;
}

std::string MysqlTable::getTable()
{
	return this->m_table;
}

std::vector<MysqlField>& MysqlTable::getFields()
{
	return this->m_fields;
}
