#include "gateway_mgr.h"

#include "model_user.h"

namespace APie {

void GatewayMgr::init()
{
	APie::RPC::rpcInit();

	Api::OpcodeHandlerSingleton::get().server.setDefaultFunc(GatewayMgr::handleDefaultOpcodes);

	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_LogicCmd, GatewayMgr::onLogicCommnad);
}

void GatewayMgr::start()
{
}

void GatewayMgr::exit()
{

}

void GatewayMgr::handleDefaultOpcodes(uint64_t serialNum, uint32_t opcodes, const std::string& msg)
{
	::rpc_msg::PRC_Multiplexer_Forward_Args args;
	args.set_opcodes(opcodes);
	args.set_body_msg(msg);

	::rpc_msg::CHANNEL server;
	server.set_type(common::EPT_Scene_Server);
	server.set_id(1);

	auto rpcCB = [](const rpc_msg::STATUS& status, const std::string& replyData)
	{
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			return;
		}
	};
	APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_Multiplexer_Forward, args, rpcCB);
}

void GatewayMgr::onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg)
{
	static std::map<std::string, MysqlTable> loadedTable;
	static ModelUser loadedUser;

	auto& command = dynamic_cast<::pubsub::LOGIC_CMD&>(msg);
	if (command.cmd() == "mysql_desc")
	{
		::mysql_proxy_msg::MysqlDescribeRequest args;

		if (command.params_size() < 2)
		{
			return;
		}

		std::string tableName = command.params()[0];
		uint64_t userId = std::stoull(command.params()[1]);

		auto ptrAdd = args.add_names();
		*ptrAdd = tableName;

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto rpcCB = [tableName, userId](const rpc_msg::STATUS& status, const std::string& replyData)
		{
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}

			::mysql_proxy_msg::MysqlDescribeResponse response;
			if (!response.ParseFromString(replyData))
			{
				return;
			}

			std::stringstream ss;
			ss << response.ShortDebugString();
			ASYNC_PIE_LOG("mysql_desc", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());

			ModelUser user;

			auto roleDesc = (*response.mutable_tables())[tableName];
			MysqlTable table;
			table = DeclarativeBase::convertFrom(roleDesc);
			user.initMetaData(table);
			bool bResult = user.checkInvalid();
			if (bResult)
			{
				loadedTable[tableName] = table;
			}

			user.fields.user_id = userId;

			mysql_proxy_msg::MysqlQueryRequest queryRequest;
			queryRequest = user.generateQuery();

			::rpc_msg::CHANNEL server;
			server.set_type(common::EPT_DB_Proxy);
			server.set_id(1);

			auto queryCB = [user](const rpc_msg::STATUS& status, const std::string& replyData) mutable
			{
				if (status.code() != ::rpc_msg::CODE_Ok)
				{
					return;
				}

				::mysql_proxy_msg::MysqlQueryResponse response;
				if (!response.ParseFromString(replyData))
				{
					return;
				}

				std::stringstream ss;
				ss << response.ShortDebugString();
				ASYNC_PIE_LOG("mysql_query", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());

				bool bResult = user.loadFromPb(response);
				if (bResult)
				{
					loadedUser = user;
				}
			};
			APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_MysqlQuery, queryRequest, queryCB);
		};
		APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_MysqlDescTable, args, rpcCB);
	}
	else if (command.cmd() == "mysql_insert")
	{
		if (command.params_size() < 2)
		{
			return;
		}

		std::string tableName = command.params()[0];
		uint64_t userId = std::stoull(command.params()[1]);

		auto findIte = loadedTable.find(tableName);
		if (findIte == loadedTable.end())
		{
			return;
		}

		//ModelUser user;
		loadedUser.fields.user_id = userId;
		loadedUser.initMetaData(findIte->second);
		bool bResult = loadedUser.checkInvalid();
		if (!bResult)
		{
			return;
		}

		mysql_proxy_msg::MysqlInsertRequest insertRequest = loadedUser.generateInsert();

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto insertCB = [](const rpc_msg::STATUS& status, const std::string& replyData) mutable
		{
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}

			::mysql_proxy_msg::MysqlInsertResponse response;
			if (!response.ParseFromString(replyData))
			{
				return;
			}

			std::stringstream ss;
			ss << response.ShortDebugString();
			ASYNC_PIE_LOG("mysql_insert", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());

		};
		APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_MysqlInsert, insertRequest, insertCB);
	}
	else if (command.cmd() == "mysql_update")
	{
		if (command.params_size() < 2)
		{
			return;
		}

		std::string tableName = command.params()[0];
		uint64_t userId = std::stoull(command.params()[1]);

		auto findIte = loadedTable.find(tableName);
		if (findIte == loadedTable.end())
		{
			return;
		}

		//ModelUser user;
		loadedUser.fields.user_id = userId;
		loadedUser.initMetaData(findIte->second);
		bool bResult = loadedUser.checkInvalid();
		if (!bResult)
		{
			return;
		}

		loadedUser.dirtySet();
		mysql_proxy_msg::MysqlUpdateRequest insertRequest = loadedUser.generateUpdate();

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto insertCB = [](const rpc_msg::STATUS& status, const std::string& replyData) mutable
		{
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}

			::mysql_proxy_msg::MysqlUpdateResponse response;
			if (!response.ParseFromString(replyData))
			{
				return;
			}

			std::stringstream ss;
			ss << response.ShortDebugString();
			ASYNC_PIE_LOG("mysql_update", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());

		};
		APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_MysqlUpdate, insertRequest, insertCB);
	}
}

}

