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

bool MysqlTable::generateQuerySQL(const ::mysql_proxy_msg::MysqlQueryRequest& query, std::string& sql)
{
	std::stringstream ss;
	ss << "SELECT ";

	uint32_t iTotalSize = m_fields.size();

	uint32_t iIndex = 0;
	for (auto &items : m_fields)
	{
		iIndex++;
		ss << "`" << items.getName() << "`";

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
		ss << "`" << optName.value() << "`" << "=" << " ";
		ss << DeclarativeBase::toString(items.value());
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
