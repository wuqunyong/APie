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
		std::tuple<uint32_t, std::string> init();
		std::tuple<uint32_t, std::string> start();
		std::tuple<uint32_t, std::string> ready();
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
