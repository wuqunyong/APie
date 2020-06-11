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
	this->m_nameIndex[field.getName()] = field.getIndex();
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

std::optional<uint32_t> MysqlTable::getIndexByName(const std::string& name)
{
	auto findIte = m_nameIndex.find(name);
	if (findIte == m_nameIndex.end())
	{
		return std::nullopt;
	}

	return std::make_optional(findIte->second);
}
