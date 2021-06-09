#include "table_cache_mgr.h"

namespace APie {

std::shared_ptr<MysqlTable> TableCacheMgr::getTable(const std::string name)
{
	auto findIte = m_tables.find(name);
	if (findIte == m_tables.end())
	{
		return nullptr;
	}

	return findIte->second;
}

void TableCacheMgr::addTable(MysqlTable& table)
{
	auto sharedPtr = std::make_shared<MysqlTable>(table);
	m_tables[table.getTable()] = sharedPtr;
}

void TableCacheMgr::delTable(const std::string name)
{
	auto findIte = m_tables.find(name);
	if (findIte != m_tables.end())
	{
		m_tables.erase(findIte);
	}
}

mysql_proxy_msg::MysqlDescTable TableCacheMgr::convertFrom(std::shared_ptr<MysqlTable> ptrTable)
{
	mysql_proxy_msg::MysqlDescTable descTable;
	if (ptrTable == nullptr)
	{
		descTable.set_result(false);
		return descTable;
	}

	descTable.set_result(true);
	descTable.set_db_name(ptrTable->getDb());
	descTable.set_table_name(ptrTable->getTable());

	for (auto& fields : ptrTable->getFields())
	{
		auto ptrAdd = descTable.add_fields();
		ptrAdd->set_index(fields.getIndex());
		ptrAdd->set_name(fields.getName());
		ptrAdd->set_flags(fields.getFlags());
		ptrAdd->set_type(fields.getType());
		ptrAdd->set_offset(fields.getOffset());
	}

	return descTable;
}

}

