#include "dbproxy_mgr.h"

#include "table_cache_mgr.h"

namespace APie {

void DBProxyMgr::init()
{
	APie::RPC::rpcInit();
	APie::RPC::RpcServerSingleton::get().registerOpcodes(rpc_msg::RPC_MysqlDescTable, DBProxyMgr::RPC_handleMysqlDescTable);
	APie::RPC::RpcServerSingleton::get().registerOpcodes(rpc_msg::RPC_MysqlQuery, DBProxyMgr::RPC_handleMysqlQuery);
	APie::RPC::RpcServerSingleton::get().registerOpcodes(rpc_msg::RPC_MysqlInsert, DBProxyMgr::RPC_handleMysqlInsert);
	APie::RPC::RpcServerSingleton::get().registerOpcodes(rpc_msg::RPC_MysqlUpdate, DBProxyMgr::RPC_handleMysqlUpdate);
	APie::RPC::RpcServerSingleton::get().registerOpcodes(rpc_msg::RPC_MysqlDelete, DBProxyMgr::RPC_handleMysqlDelete);
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

	if (request.names_size() == 0)
	{
		std::stringstream ss;
		ss << "names_size:" << request.names_size();
		response.set_result(false);
		response.set_error_info(ss.str());
		return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
	}

	for (const auto& items : request.names())
	{
		mysql_proxy_msg::MysqlDescTable descTable;

		auto sharedPtrTable = TableCacheMgrSingleton::get().getTable(items);
		if (sharedPtrTable)
		{
			descTable = TableCacheMgrSingleton::get().convertFrom(sharedPtrTable);
			(*response.mutable_tables())[items] = descTable;
			continue;
		}


		MysqlTable table;
		bool bSQL = ptrDispatched->getMySQLConnector().describeTable(items, table);
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

			TableCacheMgrSingleton::get().addTable(table);
		}
		else
		{
			response.set_result(false);
			response.set_error_info(ptrDispatched->getMySQLConnector().getError());
			return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
		}

		(*response.mutable_tables())[items] = descTable;
	}

	response.set_result(true);
	return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
}

std::tuple<uint32_t, std::string> DBProxyMgr::RPC_handleMysqlQuery(const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args)
{
	::mysql_proxy_msg::MysqlQueryResponse response;

	::mysql_proxy_msg::MysqlQueryRequest request;
	if (!request.ParseFromString(args))
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}

	std::shared_ptr<MysqlTable> sharedTable = TableCacheMgrSingleton::get().getTable(request.table_name());
	if (sharedTable == nullptr)
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}

	auto ptrDispatched = CtxSingleton::get().getLogicThread();
	if (ptrDispatched == nullptr)
	{
		return std::make_tuple(::rpc_msg::CODE_LogicThreadNull, response.SerializeAsString());
	}

	std::string sSQL;
	bool bResult = sharedTable->generateQuerySQL(ptrDispatched->getMySQLConnector(), request, sSQL);
	response.set_sql_statement(sSQL);
	if (!bResult)
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}

	std::shared_ptr<ResultSet> recordSet;
	bResult = ptrDispatched->getMySQLConnector().query(sSQL.c_str(), sSQL.length(), recordSet);
	response = DeclarativeBase::convertFrom(*sharedTable, recordSet);
	response.set_result(bResult);
	if (!bResult)
	{
		response.set_error_info(ptrDispatched->getMySQLConnector().getError());
	}
	return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
}

std::tuple<uint32_t, std::string> DBProxyMgr::RPC_handleMysqlInsert(const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args)
{
	::mysql_proxy_msg::MysqlInsertResponse response;

	::mysql_proxy_msg::MysqlInsertRequest request;
	if (!request.ParseFromString(args))
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}

	std::shared_ptr<MysqlTable> sharedTable = TableCacheMgrSingleton::get().getTable(request.table_name());
	if (sharedTable == nullptr)
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}
	
	auto ptrDispatched = CtxSingleton::get().getLogicThread();
	if (ptrDispatched == nullptr)
	{
		return std::make_tuple(::rpc_msg::CODE_LogicThreadNull, response.SerializeAsString());
	}

	std::string sSQL;
	bool bResult = sharedTable->generateInsertSQL(ptrDispatched->getMySQLConnector(), request, sSQL);
	response.set_sql_statement(sSQL);
	if (!bResult)
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}

	std::shared_ptr<ResultSet> recordSet;
	bResult = ptrDispatched->getMySQLConnector().query(sSQL.c_str(), sSQL.length(), recordSet);
	response.set_result(bResult);
	if (!bResult)
	{
		response.set_error_info(ptrDispatched->getMySQLConnector().getError());
	}
	response.set_affected_rows(ptrDispatched->getMySQLConnector().getAffectedRows());
	response.set_insert_id(ptrDispatched->getMySQLConnector().getInsertId());

	return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
}

std::tuple<uint32_t, std::string> DBProxyMgr::RPC_handleMysqlUpdate(const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args)
{
	::mysql_proxy_msg::MysqlUpdateResponse response;

	::mysql_proxy_msg::MysqlUpdateRequest request;
	if (!request.ParseFromString(args))
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}

	std::shared_ptr<MysqlTable> sharedTable = TableCacheMgrSingleton::get().getTable(request.table_name());
	if (sharedTable == nullptr)
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}

	auto ptrDispatched = CtxSingleton::get().getLogicThread();
	if (ptrDispatched == nullptr)
	{
		return std::make_tuple(::rpc_msg::CODE_LogicThreadNull, response.SerializeAsString());
	}

	std::string sSQL;
	bool bResult = sharedTable->generateUpdateSQL(ptrDispatched->getMySQLConnector(), request, sSQL);
	response.set_sql_statement(sSQL);
	if (!bResult)
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}

	std::shared_ptr<ResultSet> recordSet;
	bResult = ptrDispatched->getMySQLConnector().query(sSQL.c_str(), sSQL.length(), recordSet);
	response.set_result(bResult);
	if (!bResult)
	{
		response.set_error_info(ptrDispatched->getMySQLConnector().getError());
	}
	response.set_affected_rows(ptrDispatched->getMySQLConnector().getAffectedRows());
	response.set_insert_id(ptrDispatched->getMySQLConnector().getInsertId());

	return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
}

std::tuple<uint32_t, std::string> DBProxyMgr::RPC_handleMysqlDelete(const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args)
{
	::mysql_proxy_msg::MysqlDeleteResponse response;

	::mysql_proxy_msg::MysqlDeleteRequest request;
	if (!request.ParseFromString(args))
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}

	std::shared_ptr<MysqlTable> sharedTable = TableCacheMgrSingleton::get().getTable(request.table_name());
	if (sharedTable == nullptr)
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}

	auto ptrDispatched = CtxSingleton::get().getLogicThread();
	if (ptrDispatched == nullptr)
	{
		return std::make_tuple(::rpc_msg::CODE_LogicThreadNull, response.SerializeAsString());
	}

	std::string sSQL;
	bool bResult = sharedTable->generateDeleteSQL(ptrDispatched->getMySQLConnector(), request, sSQL);
	response.set_sql_statement(sSQL);
	if (!bResult)
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}

	std::shared_ptr<ResultSet> recordSet;
	bResult = ptrDispatched->getMySQLConnector().query(sSQL.c_str(), sSQL.length(), recordSet);
	response.set_result(bResult);
	if (!bResult)
	{
		response.set_error_info(ptrDispatched->getMySQLConnector().getError());
	}
	response.set_affected_rows(ptrDispatched->getMySQLConnector().getAffectedRows());

	return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
}

}

