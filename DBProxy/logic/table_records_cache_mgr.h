#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

#include "apie.h"


namespace APie {

	class TableRecords
	{

	};

	class TableRecordsCacheMgr
	{
	public:
		std::shared_ptr<MysqlTable> getRecord(const std::string key);
		void addRecord(const std::string key, MysqlTable& table);
		void delRecord(const std::string key);

	private:
		std::map<std::string, std::shared_ptr<MysqlTable>> m_tables;
	};

	using TableRecordsCacheMgrSingleton = ThreadSafeSingleton<TableRecordsCacheMgr>;
}
