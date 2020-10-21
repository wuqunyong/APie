#include "gateway_mgr.h"
#include "../../SharedDir/dao/model_user.h"
#include "../../LibAPie/common/message_traits.h"


namespace APie {

	std::tuple<uint32_t, std::string> GatewayMgr::init()
{
	APie::RPC::rpcInit();

	Api::OpcodeHandlerSingleton::get().server.setDefaultFunc(GatewayMgr::handleDefaultOpcodes);

	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_LogicCmd, GatewayMgr::onLogicCommnad);

	return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
}


std::tuple<uint32_t, std::string> GatewayMgr::start()
{
	auto dbType = DeclarativeBase::DBType::DBT_Role;
	DAOFactoryTypeSingleton::get().registerRequiredTable(dbType, ModelUser::getFactoryName(), ModelUser::createMethod);

	auto requiredTableOpt = DAOFactoryTypeSingleton::get().getRequiredTable(dbType);
	if (!requiredTableOpt.has_value())
	{
		return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
	}


	::rpc_msg::CHANNEL server;
	server.set_type(common::EPT_DB_Proxy);
	server.set_id(1);

	std::vector<std::string> tables;
	for (const auto& items : requiredTableOpt.value())
	{
		tables.push_back(items.first);
	}

	auto ptrReadyCb = [](bool bResul, std::string sInfo, uint64_t iCallCount)
	{
		if (!bResul)
		{
			std::stringstream ss;
			ss << "CallMysqlDescTable|bResul:" << bResul << ",sInfo:" << sInfo << ",iCallCount:" << iCallCount;

			fatalExit(ss.str().c_str());
		}

		std::stringstream ss;
		ss << "Server Ready!" << std::endl;

		std::cout << ss.str();
		ASYNC_PIE_LOG("ServerStatus", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());

	};
	uint64_t iCallCount = 0;
	CallMysqlDescTable(server, DeclarativeBase::DBType::DBT_Role, tables, iCallCount, ptrReadyCb);

	return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
}

std::tuple<uint32_t, std::string> GatewayMgr::ready()
{
	return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
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

				DAOFactoryTypeSingleton::get().addLoadedTable(DeclarativeBase::DBType::DBT_Role, tableName, table);
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

				//bool bResult = user.loadFromPb(response);
				//if (bResult)
				//{
				//	loadedUser = user;
				//}

				bool bResult = user.loadFromPbCheck(response);
				if (!bResult)
				{
					return;
				}

				uint32_t iRowCount = response.table().rows_size();
				for (auto& rowData : response.table().rows())
				{
					user.loadFromPb(rowData);

					loadedUser = user;
					break;
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
	else if (command.cmd() == "mysql_delete")
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

		mysql_proxy_msg::MysqlDeleteRequest insertRequest = loadedUser.generateDelete();

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto insertCB = [](const rpc_msg::STATUS& status, const std::string& replyData) mutable
		{
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}

			::mysql_proxy_msg::MysqlDeleteResponse response;
			if (!response.ParseFromString(replyData))
			{
				return;
			}

			std::stringstream ss;
			ss << response.ShortDebugString();
			ASYNC_PIE_LOG("mysql_delete", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());

		};
		APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_MysqlDelete, insertRequest, insertCB);
	}
	else if (command.cmd() == "multi_mysql_desc")
	{
		if (command.params_size() < 1)
		{
			return;
		}

		std::vector<std::tuple<::rpc_msg::CHANNEL, ::rpc_msg::RPC_OPCODES, std::string>> methods;

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		for (const auto& items : command.params())
		{
			::mysql_proxy_msg::MysqlDescribeRequest args;

			std::string tableName = items;

			auto ptrAdd = args.add_names();
			*ptrAdd = tableName;

			methods.push_back(std::make_tuple(server, ::rpc_msg::RPC_MysqlDescTable, args.SerializeAsString()));
		}

		auto rpcCB = [](const rpc_msg::STATUS& status, std::vector<std::tuple<rpc_msg::STATUS, std::string>> replyData)
		{
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}

		};
		APie::RPC::RpcClientSingleton::get().multiCallByRoute(methods, rpcCB);
	}
	else if (command.cmd() == "load_from_db")
	{
		if (command.params_size() < 1)
		{
			return;
		}

		uint64_t userId = std::stoull(command.params()[0]);

		ModelUser user;
		user.fields.user_id = userId;

		bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto cb = [](rpc_msg::STATUS status, ModelUser user, uint32_t iRows) {
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}
		};
		LoadFromDb<ModelUser>(server, user, cb);
	}
	else if (command.cmd() == "load_from_db_by_filter")
	{
		if (command.params_size() < 2)
		{
			return;
		}

		uint64_t gameId = std::stoull(command.params()[0]);
		uint32_t level = std::stoull(command.params()[1]);


		ModelUser user;
		user.fields.game_id = gameId;
		user.fields.level = level;
		bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());
		user.markFilter({ 1, 2 });

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto cb = [](rpc_msg::STATUS status, std::vector<ModelUser>& userList) {
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}
		};
		LoadFromDbByFilter<ModelUser>(server, user, cb);
	}
	else if (command.cmd() == "mysql_insert_orm")
	{
		if (command.params_size() < 2)
		{
			return;
		}

		uint64_t userId = std::stoull(command.params()[0]);
		uint32_t level = std::stoull(command.params()[1]);


		ModelUser user;
		user.fields.user_id = userId;
		user.fields.level = level;
		bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto cb = [](rpc_msg::STATUS status, bool result, uint64_t affectedRows, uint64_t insertId) {
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}
		};
		InsertToDb<ModelUser>(server, user, cb);
	}
	else if (command.cmd() == "mysql_update_orm")
	{
		if (command.params_size() < 2)
		{
			return;
		}

		uint64_t userId = std::stoull(command.params()[0]);
		uint32_t level = std::stoull(command.params()[1]);


		ModelUser user;
		user.fields.user_id = userId;
		user.fields.level = level;
		bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());
		user.markDirty({ 2 });

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto cb = [](rpc_msg::STATUS status, bool result, uint64_t affectedRows) {
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}
		};
		UpdateToDb<ModelUser>(server, user, cb);
	}
	else if (command.cmd() == "mysql_delete_orm")
	{
		if (command.params_size() < 1)
		{
			return;
		}

		uint64_t userId = std::stoull(command.params()[0]);

		ModelUser user;
		user.fields.user_id = userId;
		bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto cb = [](rpc_msg::STATUS status, bool result, uint64_t affectedRows) {
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}
		};
		DeleteFromDb<ModelUser>(server, user, cb);
	}
}

}

