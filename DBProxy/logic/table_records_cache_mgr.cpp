#include "table_records_cache_mgr.h"

namespace APie {

std::shared_ptr<MysqlTable> TableRecordsCacheMgr::getRecord(const std::string key)
{
	auto findIte = m_tables.find(key);
	if (findIte == m_tables.end())
	{
		return nullptr;
	}

	return findIte->second;
}

void TableRecordsCacheMgr::addRecord(const std::string key, MysqlTable& table)
{
	auto sharedPtr = std::make_shared<MysqlTable>(table);
	m_tables[key] = sharedPtr;
}

void TableRecordsCacheMgr::delRecord(const std::string key)
{
	auto findIte = m_tables.find(key);
	if (findIte != m_tables.end())
	{
		m_tables.erase(findIte);
	}
}


}

