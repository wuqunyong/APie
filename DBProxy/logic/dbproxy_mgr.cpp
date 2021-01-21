#include "dbproxy_mgr.h"

#include "table_cache_mgr.h"

#include "../../LibAPie/rpc/server/rpc_server.h"

#include "../../SharedDir/dao/model_account.h"
#include "../../SharedDir/dao/model_user.h"

namespace APie {

std::tuple<uint32_t, std::string> DBProxyMgr::init()
{
	auto type = APie::CtxSingleton::get().getServerType();

	std::set<uint32_t> validType;
	validType.insert(common::EPT_DB_ACCOUNT_Proxy);
	validType.insert(common::EPT_DB_ROLE_Proxy);

	if (validType.count(type) == 0)
	{
		return std::make_tuple(Hook::HookResult::HR_Error, "invalid Type");
	}

	// CMD
	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_LogicCmd, DBProxyMgr::onLogicCommnad);

	LogicCmdHandlerSingleton::get().init();

	// RPC
	APie::RPC::rpcInit();
	APie::RPC::RpcServerSingleton::get().registerOpcodes<::mysql_proxy_msg::MysqlDescribeRequest>(rpc_msg::RPC_MysqlDescTable, DBProxyMgr::RPC_handleMysqlDescTable);
	APie::RPC::RpcServerSingleton::get().registerOpcodes<::mysql_proxy_msg::MysqlQueryRequest>(rpc_msg::RPC_MysqlQuery, DBProxyMgr::RPC_handleMysqlQuery);
	APie::RPC::RpcServerSingleton::get().registerOpcodes<::mysql_proxy_msg::MysqlInsertRequest>(rpc_msg::RPC_MysqlInsert, DBProxyMgr::RPC_handleMysqlInsert);
	APie::RPC::RpcServerSingleton::get().registerOpcodes<::mysql_proxy_msg::MysqlUpdateRequest>(rpc_msg::RPC_MysqlUpdate, DBProxyMgr::RPC_handleMysqlUpdate);
	APie::RPC::RpcServerSingleton::get().registerOpcodes<::mysql_proxy_msg::MysqlDeleteRequest>(rpc_msg::RPC_MysqlDelete, DBProxyMgr::RPC_handleMysqlDelete);
	APie::RPC::RpcServerSingleton::get().registerOpcodes<::mysql_proxy_msg::MysqlQueryRequestByFilter>(rpc_msg::RPC_MysqlQueryByFilter, DBProxyMgr::RPC_handleMysqlQueryByFilter);

	return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
}

std::tuple<uint32_t, std::string> DBProxyMgr::start()
{
	DeclarativeBase::DBType dbType = DeclarativeBase::DBType::DBT_None;

	auto type = APie::CtxSingleton::get().getServerType();
	switch (type)
	{
	case common::EPT_DB_ACCOUNT_Proxy:
	{
		dbType = DeclarativeBase::DBType::DBT_Account;
		DAOFactoryTypeSingleton::get().registerRequiredTable(dbType, ModelAccount::getFactoryName(), ModelAccount::createMethod);

		break;
	}
	case common::EPT_DB_ROLE_Proxy:
	{
		dbType = DeclarativeBase::DBType::DBT_Role;
		DAOFactoryTypeSingleton::get().registerRequiredTable(dbType, ModelUser::getFactoryName(), ModelUser::createMethod);

		break;
	}
	default:
		return std::make_tuple(Hook::HookResult::HR_Error, "invalid Type");
	}

	auto ptrDispatched = CtxSingleton::get().getLogicThread();
	if (ptrDispatched == nullptr)
	{
		return std::make_tuple(Hook::HookResult::HR_Error, "null ptrDispatched");
	}

	auto requiredTableOpt = DAOFactoryTypeSingleton::get().getRequiredTable(dbType);
	if (!requiredTableOpt.has_value())
	{
		return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
	}

	std::vector<std::string> tables;
	for (const auto& items : requiredTableOpt.value())
	{
		tables.push_back(items.first);
	}

	for (const auto& tableName : tables)
	{
		MysqlTable table;
		bool bSQL = ptrDispatched->getMySQLConnector().describeTable(tableName, table);
		if (bSQL)
		{
			TableCacheMgrSingleton::get().addTable(table);

			auto ptrDaoBase = DAOFactoryTypeSingleton::get().getCreateFunc(dbType, tableName);
			if (ptrDaoBase == nullptr)
			{
				std::stringstream ss;
				ss << "tableName:" << tableName << " not declare";

				return std::make_tuple(Hook::HookResult::HR_Error, ss.str());
			}

			ptrDaoBase->initMetaData(table);
			bool bResult = ptrDaoBase->checkInvalid();
			if (!bResult)
			{
				std::stringstream ss;
				ss << "tableName:" << tableName << " checkInvalid false";

				return std::make_tuple(Hook::HookResult::HR_Error, ss.str());
			}
		}
		else
		{
			return std::make_tuple(Hook::HookResult::HR_Error, ptrDispatched->getMySQLConnector().getError());
		}
	}

	APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);

	return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
}

std::tuple<uint32_t, std::string> DBProxyMgr::ready()
{
	std::stringstream ss;
	ss << "Server Ready!";
	std::cout << ss.str() << std::endl;
	ASYNC_PIE_LOG("ServerStatus", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

void DBProxyMgr::exit()
{

}

void DBProxyMgr::onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg)
{
	auto& command = dynamic_cast<::pubsub::LOGIC_CMD&>(msg);
	auto handlerOpt = LogicCmdHandlerSingleton::get().findCb(command.cmd());
	if (!handlerOpt.has_value())
	{
		return;
	}

	handlerOpt.value()(command);
}

std::tuple<uint32_t, std::string> DBProxyMgr::RPC_handleMysqlDescTable(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::mysql_proxy_msg::MysqlDescribeRequest& request)
{
	::mysql_proxy_msg::MysqlDescribeResponse response;

	auto ptrDispatched = CtxSingleton::get().getLogicThread();
	if (ptrDispatched == nullptr)
	{
		std::stringstream ss;
		ss << "LogicThread Null";
		response.set_result(true);
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


		//MysqlTable table;
		//bool bSQL = ptrDispatched->getMySQLConnector().describeTable(items, table);
		//descTable.set_result(bSQL);
		//if (bSQL)
		//{
		//	descTable.set_db_name(table.getDb());
		//	descTable.set_table_name(table.getTable());

		//	for (auto& fields : table.getFields())
		//	{
		//		auto ptrAdd = descTable.add_fields();
		//		ptrAdd->set_index(fields.getIndex());
		//		ptrAdd->set_name(fields.getName());
		//		ptrAdd->set_flags(fields.getFlags());
		//		ptrAdd->set_type(fields.getType());
		//		ptrAdd->set_offset(fields.getOffset());
		//	}

		//	TableCacheMgrSingleton::get().addTable(table);
		//}
		//else
		//{
		//	response.set_result(false);
		//	response.set_error_info(ptrDispatched->getMySQLConnector().getError());
		//	return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
		//}
		
		//(*response.mutable_tables())[items] = descTable;

		std::stringstream ss;
		ss << "not cache:" << items;

		response.set_result(false);
		response.set_error_info(ss.str());
		return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
	}

	response.set_result(true);
	return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
}

std::tuple<uint32_t, std::string> DBProxyMgr::RPC_handleMysqlQuery(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::mysql_proxy_msg::MysqlQueryRequest& request)
{
	::mysql_proxy_msg::MysqlQueryResponse response;

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
	response.set_sql_statement(sSQL);
	response.set_result(bResult);
	if (!bResult)
	{
		response.set_error_info(ptrDispatched->getMySQLConnector().getError());
	}
	return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
}

std::tuple<uint32_t, std::string> DBProxyMgr::RPC_handleMysqlQueryByFilter(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::mysql_proxy_msg::MysqlQueryRequestByFilter& request)
{
	::mysql_proxy_msg::MysqlQueryResponse response;

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
	bool bResult = sharedTable->generateQueryByFilterSQL(ptrDispatched->getMySQLConnector(), request, sSQL);
	response.set_sql_statement(sSQL);
	if (!bResult)
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, response.SerializeAsString());
	}

	std::shared_ptr<ResultSet> recordSet;
	bResult = ptrDispatched->getMySQLConnector().query(sSQL.c_str(), sSQL.length(), recordSet);

	response.mutable_table()->set_db(sharedTable->getDb());
	response.mutable_table()->set_name(sharedTable->getTable());
	response.set_result(bResult);
	if (!bResult)
	{
		response.set_error_info(ptrDispatched->getMySQLConnector().getError());
	}

	if (!recordSet)
	{
		return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
	}

	const uint32_t iBatchSize = 3;
	uint32_t iCurBatchSize = 0;
	uint32_t iOffset = 0;

	do 
	{
		auto optRowData = DeclarativeBase::convertToRowFrom(*sharedTable, recordSet);
		if (optRowData.has_value())
		{
			auto ptrAddRows = response.mutable_table()->add_rows();
			*ptrAddRows = optRowData.value();

			iCurBatchSize++;
			if (iCurBatchSize >= iBatchSize)
			{
				iCurBatchSize = 0;

				RPC::RpcServerSingleton::get().asyncStreamReply(client, ::rpc_msg::CODE_Ok, response.SerializeAsString(), true, iOffset);
				response.mutable_table()->clear_rows();
			}
		}
		else
		{
			break;
		}

		iOffset++;
	} while (true);

	RPC::RpcServerSingleton::get().asyncStreamReply(client, ::rpc_msg::CODE_Ok, response.SerializeAsString(), false, iOffset);
	return std::make_tuple(::rpc_msg::CODE_Ok_Async, "");
}


std::tuple<uint32_t, std::string> DBProxyMgr::RPC_handleMysqlInsert(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::mysql_proxy_msg::MysqlInsertRequest& request)
{
	::mysql_proxy_msg::MysqlInsertResponse response;

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

std::tuple<uint32_t, std::string> DBProxyMgr::RPC_handleMysqlUpdate(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::mysql_proxy_msg::MysqlUpdateRequest& request)
{
	::mysql_proxy_msg::MysqlUpdateResponse response;

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

std::tuple<uint32_t, std::string> DBProxyMgr::RPC_handleMysqlDelete(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::mysql_proxy_msg::MysqlDeleteRequest& request)
{
	::mysql_proxy_msg::MysqlDeleteResponse response;

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

