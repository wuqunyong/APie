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

	class TableCacheMgr
	{
	public:
		std::shared_ptr<MysqlTable> getTable(const std::string name);
		void addTable(MysqlTable& table);
		void delTable(const std::string name);

		mysql_proxy_msg::MysqlDescTable convertFrom(std::shared_ptr<MysqlTable> ptrTable);

	private:
		std::map<std::string, std::shared_ptr<MysqlTable>> m_tables;
	};

	using TableCacheMgrSingleton = ThreadSafeSingleton<TableCacheMgr>;
}
