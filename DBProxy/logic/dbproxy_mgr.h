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
		// CMD
		static void onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg);

		// RPC
		static std::tuple<uint32_t, std::string> RPC_handleMysqlDescTable(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::mysql_proxy_msg::MysqlDescribeRequest& request);
		static std::tuple<uint32_t, std::string> RPC_handleMysqlQuery(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::mysql_proxy_msg::MysqlQueryRequest& request);
		static std::tuple<uint32_t, std::string> RPC_handleMysqlInsert(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::mysql_proxy_msg::MysqlInsertRequest& request);
		static std::tuple<uint32_t, std::string> RPC_handleMysqlUpdate(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::mysql_proxy_msg::MysqlUpdateRequest& request);
		static std::tuple<uint32_t, std::string> RPC_handleMysqlDelete(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::mysql_proxy_msg::MysqlDeleteRequest& request);
		static std::tuple<uint32_t, std::string> RPC_handleMysqlQueryByFilter(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::mysql_proxy_msg::MysqlQueryRequestByFilter& request);
		

	private:
	};

	using DBProxyMgrSingleton = ThreadSafeSingleton<DBProxyMgr>;
}
