#include "mysql_table.h"

#include "mysql_orm.h"

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
	this->m_indexName[field.getIndex()] = field.getName();
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

std::optional<std::string> MysqlTable::getNameByIndex(uint32_t index)
{
	auto findIte = m_indexName.find(index);
	if (findIte == m_indexName.end())
	{
		return std::nullopt;
	}

	return std::make_optional(findIte->second);
}

bool MysqlTable::generateQuerySQL(MySQLConnector& connector, const ::mysql_proxy_msg::MysqlQueryRequest& query, std::string& sql)
{
	const std::string graveAccent("`");

	std::stringstream ss;
	ss << "SELECT ";

	uint32_t iTotalSize = m_fields.size();

	uint32_t iIndex = 0;
	for (auto &items : m_fields)
	{
		iIndex++;
		ss << graveAccent << items.getName() << graveAccent;

		if (iIndex < iTotalSize)
		{
			ss << ",";
		}
		else
		{
			ss << " ";
		}
	}

	ss << "FROM " << m_table << " ";
	ss << "WHERE ";

	bool bFirst = true;
	for (auto& items : query.primary_key())
	{
		if (bFirst)
		{
			bFirst = false;
		}
		else
		{
			ss << " AND ";
		}

		std::string fieldName;
		auto optName = this->getNameByIndex(items.index());
		if (!optName.has_value())
		{
			std::stringstream errorInfo;
			errorInfo << "invalidName|table:" << m_table << "|index:" << items.index();

			sql = errorInfo.str();
			return false;
		}
		ss << graveAccent << optName.value() << graveAccent << "=" << " ";
		ss << DeclarativeBase::toString(connector, items.value());
	}

	if (bFirst)
	{
		std::stringstream errorInfo;
		errorInfo << "no primary";
		sql = errorInfo.str();

		return false;
	}

	sql = ss.str();
	return true;
}


bool MysqlTable::generateInsertSQL(MySQLConnector& connector, const ::mysql_proxy_msg::MysqlInsertRequest& query, std::string& sql)
{
	const std::string graveAccent("`");
	const std::string quote("'");

	if (m_fields.size() != query.fields_size())
	{
		std::stringstream ss;
		ss << "fieldSize not equal|fields:" << m_fields.size() << "|insertFieldSize:" << query.fields_size();
		sql = ss.str();
		return false;
	}

	std::stringstream ss;
	ss << "INSERT INTO " << graveAccent << query.table_name() << graveAccent << " (";

	uint32_t iTotalSize = m_fields.size();

	uint32_t iIndex = 0;
	for (auto &items : m_fields)
	{
		iIndex++;
		ss << graveAccent << items.getName() << graveAccent;

		if (iIndex < iTotalSize)
		{
			ss << ",";
		}
		else
		{
			ss << "";
		}
	}

	ss << " ) VALUES (" ;

	iTotalSize = query.fields_size();
	iIndex = 0;
	for (auto& items : query.fields())
	{
		iIndex++;
		ss << DeclarativeBase::toString(connector, items.value());
		if (iIndex < iTotalSize)
		{
			ss << ",";
		}
		else
		{
			ss << "";
		}
	}
	ss << ")";

	sql = ss.str();
	return true;
}