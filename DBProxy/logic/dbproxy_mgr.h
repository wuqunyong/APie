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

	class DBProxyMgr
	{
	public:
		void init();
		std::tuple<uint32_t, std::string> start();
		void exit();

	public:
		static std::tuple<uint32_t, std::string> RPC_handleMysqlDescTable(const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args);
		static std::tuple<uint32_t, std::string> RPC_handleMysqlQuery(const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args);
		static std::tuple<uint32_t, std::string> RPC_handleMysqlInsert(const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args);
		static std::tuple<uint32_t, std::string> RPC_handleMysqlUpdate(const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args);
		static std::tuple<uint32_t, std::string> RPC_handleMysqlDelete(const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args);
		static std::tuple<uint32_t, std::string> RPC_handleMysqlQueryByFilter(const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args);
		

	private:
	};

	using DBProxyMgrSingleton = ThreadSafeSingleton<DBProxyMgr>;
}
