#include "dbproxy_mgr.h"

namespace APie {

void DBProxyMgr::init()
{
	APie::RPC::rpcInit();
	APie::RPC::RpcServerSingleton::get().registerOpcodes(rpc_msg::RPC_MysqlDescTable, DBProxyMgr::RPC_handleMysqlDescTable);
}

void DBProxyMgr::start()
{

}

void DBProxyMgr::exit()
{

}

std::tuple<uint32_t, std::string> DBProxyMgr::RPC_handleMysqlDescTable(const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args)
{
	::mysql_proxy_msg::MysqlDescribeResponse response;

	::mysql_proxy_msg::MysqlDescribeRequest request;
	if (!request.ParseFromString(args))
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}

	auto ptrDispatched = CtxSingleton::get().getLogicThread();
	if (ptrDispatched == nullptr)
	{
		return std::make_tuple(::rpc_msg::CODE_LogicThreadNull, response.SerializeAsString());
	}

	for (const auto& items : request.names())
	{
		MysqlTable table;
		bool bSQL = ptrDispatched->getMySQLConnector().describeTable(items, table);

		mysql_proxy_msg::MysqlDescTable descTable;
		descTable.set_result(bSQL);
		if (bSQL)
		{
			descTable.set_db_name(table.getDb());
			descTable.set_table_name(table.getTable());

			for (auto& fields : table.getFields())
			{
				auto ptrAdd = descTable.add_fields();
				ptrAdd->set_index(fields.getIndex());
				ptrAdd->set_name(fields.getName());
				ptrAdd->set_flags(fields.getFlags());
				ptrAdd->set_type(fields.getType());
				ptrAdd->set_offset(fields.getOffset());
			}
		}

		(*response.mutable_tables())[items] = descTable;
	}

	return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
}

}

