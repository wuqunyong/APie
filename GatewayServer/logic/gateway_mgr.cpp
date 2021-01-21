#include "gateway_mgr.h"
#include "../../SharedDir/dao/model_user.h"
#include "../../LibAPie/common/message_traits.h"
#include "../../PBMsg/login_msg.pb.h"
#include "gateway_role.h"


namespace APie {


std::tuple<uint32_t, std::string> GatewayMgr::init()
{
	auto type = APie::CtxSingleton::get().getServerType();

	std::set<uint32_t> validType;
	validType.insert(common::EPT_Gateway_Server);

	if (validType.count(type) == 0)
	{
		return std::make_tuple(Hook::HookResult::HR_Error, "invalid Type");
	}

	// CMD
	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_LogicCmd, GatewayMgr::onLogicCommnad);

	LogicCmdHandlerSingleton::get().init();
	LogicCmdHandlerSingleton::get().registerOnCmd("insert_to_db", "mysql_insert_to_db_orm", GatewayMgr::onMysqlInsertToDbORM);
	LogicCmdHandlerSingleton::get().registerOnCmd("delete_from_db", "mysql_delete_from_db_orm", GatewayMgr::onMysqlDeleteFromDbORM);
	LogicCmdHandlerSingleton::get().registerOnCmd("update_to_db", "mysql_update_to_db_orm", GatewayMgr::onMysqlUpdateToDbORM);
	LogicCmdHandlerSingleton::get().registerOnCmd("load_from_db", "mysql_load_from_db_orm", GatewayMgr::onMysqlLoadFromDbORM);
	LogicCmdHandlerSingleton::get().registerOnCmd("query_from_db", "mysql_query_from_db_orm", GatewayMgr::onMysqlQueryFromDbORM);
	

	// RPC
	APie::RPC::rpcInit();
	APie::RPC::RpcServerSingleton::get().registerOpcodes<::rpc_msg::PRC_DeMultiplexer_Forward_Args>(rpc_msg::RPC_DeMultiplexer_Forward, GatewayMgr::RPC_handleDeMultiplexerForward);


	return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
}


std::tuple<uint32_t, std::string> GatewayMgr::start()
{
	// 加载，数据表结构
	auto dbType = DeclarativeBase::DBType::DBT_Role;
	DAOFactoryTypeSingleton::get().registerRequiredTable(dbType, ModelUser::getFactoryName(), ModelUser::createMethod);

	auto requiredTableOpt = DAOFactoryTypeSingleton::get().getRequiredTable(dbType);
	if (!requiredTableOpt.has_value())
	{
		APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);

		return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
	}


	::rpc_msg::CHANNEL server;
	server.set_type(common::EPT_DB_ROLE_Proxy);
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

			PANIC_ABORT(ss.str().c_str());
		}

		APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);

	};
	CallMysqlDescTable(server, DeclarativeBase::DBType::DBT_Role, tables, ptrReadyCb);

	return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
}

std::tuple<uint32_t, std::string> GatewayMgr::ready()
{
	// PubSub
	APie::PubSubSingleton::get().subscribe(::pubsub::PT_ServerPeerClose, GatewayMgr::onServerPeerClose);


	// CLIENT OPCODE
	Api::PBHandler& serverPB = Api::OpcodeHandlerSingleton::get().server;
	serverPB.setDefaultFunc(GatewayMgr::handleDefaultOpcodes);
	serverPB.bind(::opcodes::OP_MSG_REQUEST_CLIENT_LOGIN, GatewayMgr::handleRequestClientLogin, ::login_msg::MSG_REQUEST_CLIENT_LOGIN::default_instance());


	std::stringstream ss;
	ss << "Server Ready!";
	std::cout << ss.str() << std::endl;
	ASYNC_PIE_LOG("ServerStatus", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
}

void GatewayMgr::exit()
{

}

std::shared_ptr<GatewayRole> GatewayMgr::findGatewayRoleById(uint64_t iRoleId)
{
	auto findIte = m_roleIdMapSerialNum.find(iRoleId);
	if (findIte == m_roleIdMapSerialNum.end())
	{
		return nullptr;
	}

	return findGatewayRoleBySerialNum(findIte->second);
}

std::shared_ptr<GatewayRole> GatewayMgr::findGatewayRoleBySerialNum(uint64_t iSerialNum)
{
	auto findIte = m_serialNumMap.find(iSerialNum);
	if (findIte == m_serialNumMap.end())
	{
		return nullptr;
	}

	return findIte->second;
}

std::optional<uint64_t> GatewayMgr::findRoleIdBySerialNum(uint64_t iSerialNum)
{
	auto findIte = m_serialNumMap.find(iSerialNum);
	if (findIte == m_serialNumMap.end())
	{
		return std::nullopt;
	}

	return findIte->second->getRoleId();
}

bool GatewayMgr::addGatewayRole(std::shared_ptr<GatewayRole> ptrGatewayRole)
{
	if (ptrGatewayRole == nullptr)
	{
		return false;
	}

	auto iRoleId = ptrGatewayRole->getRoleId();
	auto iSerialNum = ptrGatewayRole->getSerailNum();

	auto ptrConnection = Event::DispatcherImpl::getConnection(iSerialNum);
	if (ptrConnection != nullptr)
	{
		ptrGatewayRole->setMaskFlag(ptrConnection->getMaskFlag());
	}

	auto ptrExist = findGatewayRoleById(iRoleId);
	if (ptrExist != nullptr)
	{
		return false;
	}

	m_serialNumMap[iSerialNum] = ptrGatewayRole;
	m_roleIdMapSerialNum[iRoleId] = iSerialNum;

	return true;
}

bool GatewayMgr::removeGateWayRole(uint64_t iRoleId)
{
	auto findIte = m_roleIdMapSerialNum.find(iRoleId);
	if (findIte == m_roleIdMapSerialNum.end())
	{
		return false;
	}

	m_serialNumMap.erase(findIte->second);
	m_roleIdMapSerialNum.erase(findIte);
	return true;
}

void GatewayMgr::handleDefaultOpcodes(uint64_t serialNum, uint32_t opcodes, const std::string& msg)
{	
	auto ptrGatewayRole = GatewayMgrSingleton::get().findGatewayRoleBySerialNum(serialNum);
	if (ptrGatewayRole == nullptr)
	{
		ASYNC_PIE_LOG("handleDefaultOpcodes", PIE_CYCLE_DAY, PIE_ERROR, "Not Login|serialNum:%lld|opcodes:%d", serialNum, opcodes);
		return;
	}

	uint32_t iGWId = APie::CtxSingleton::get().getServerId();
	uint64_t iUserId = ptrGatewayRole->getRoleId();

	::rpc_msg::PRC_Multiplexer_Forward_Args args;
	args.mutable_role_id()->set_gw_id(iGWId);
	args.mutable_role_id()->set_user_id(iUserId);
	args.set_opcodes(opcodes);
	args.set_body_msg(msg);

	::rpc_msg::CHANNEL server;
	server.set_type(common::EPT_Scene_Server);
	server.set_id(1);
	 
	auto rpcCB = [opcodes, iGWId, iUserId](const rpc_msg::STATUS& status, const std::string& replyData)
	{
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			ASYNC_PIE_LOG("ForwordMsg", PIE_CYCLE_DAY, PIE_ERROR, "Forword Msg Error|code:%d|opcodes:%d|iGWId:%d|iUserId:%ll", status.code(), opcodes, iGWId, iUserId);
			return;
		}
	};
	APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_Multiplexer_Forward, args, rpcCB);
}

std::tuple<uint32_t, std::string> GatewayMgr::RPC_handleDeMultiplexerForward(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::rpc_msg::PRC_DeMultiplexer_Forward_Args& request)
{
	uint64_t iRoleId = request.role_id().user_id();
	auto ptrGatewayRole = GatewayMgrSingleton::get().findGatewayRoleById(iRoleId);
	if (ptrGatewayRole == nullptr)
	{
		return std::make_tuple(::rpc_msg::CODE_ParseError, "");
	}

	uint64_t iSerialNum = ptrGatewayRole->getSerailNum();
	uint32_t iMaskFlag = ptrGatewayRole->getMaskFlag();
	if (iMaskFlag == 0)
	{
		Network::OutputStream::sendMsgByStr(iSerialNum, request.opcodes(), request.body_msg(), APie::ConnetionType::CT_SERVER);
	}
	else
	{
		Network::OutputStream::sendMsgByStrByFlag(iSerialNum, request.opcodes(), request.body_msg(), iMaskFlag, APie::ConnetionType::CT_SERVER);
	}
	return std::make_tuple(::rpc_msg::CODE_Ok, "DeMultiplexer success");
}

void GatewayMgr::onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg)
{

	auto& command = dynamic_cast<::pubsub::LOGIC_CMD&>(msg);
	auto handlerOpt = LogicCmdHandlerSingleton::get().findCb(command.cmd());
	if (!handlerOpt.has_value())
	{
		return;
	}

	handlerOpt.value()(command);

	/*
	static std::map<std::string, MysqlTable> loadedTable;
	static ModelUser loadedUser;

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
		server.set_type(common::EPT_DB_ROLE_Proxy);
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
			server.set_type(common::EPT_DB_ROLE_Proxy);
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
		server.set_type(common::EPT_DB_ROLE_Proxy);
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
		server.set_type(common::EPT_DB_ROLE_Proxy);
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
		server.set_type(common::EPT_DB_ROLE_Proxy);
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
		server.set_type(common::EPT_DB_ROLE_Proxy);
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
	*/

}

void GatewayMgr::onMysqlLoadFromDbORM(::pubsub::LOGIC_CMD& cmd)
{
	if (cmd.params_size() < 1)
	{
		return;
	}

	uint64_t userId = std::stoull(cmd.params()[0]);

	ModelUser user;
	user.fields.user_id = userId;

	bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());
	if (!bResult)
	{
		return;
	}

	::rpc_msg::CHANNEL server;
	server.set_type(common::EPT_DB_ROLE_Proxy);
	server.set_id(1);

	auto cb = [](rpc_msg::STATUS status, ModelUser user, uint32_t iRows) {
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			return;
		}
	};
	LoadFromDb<ModelUser>(server, user, cb);
}

void GatewayMgr::onMysqlQueryFromDbORM(::pubsub::LOGIC_CMD& cmd)
{
	if (cmd.params_size() < 2)
	{
		return;
	}

	uint64_t gameId = std::stoull(cmd.params()[0]);
	uint32_t level = std::stoul(cmd.params()[1]);


	ModelUser user;
	user.fields.game_id = gameId;
	user.fields.level = level;
	bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());
	if (!bResult)
	{
		return;
	}
	user.markFilter({ 1, 2 });

	::rpc_msg::CHANNEL server;
	server.set_type(common::EPT_DB_ROLE_Proxy);
	server.set_id(1);

	auto cb = [](rpc_msg::STATUS status, std::vector<ModelUser>& userList) {
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			return;
		}
	};
	LoadFromDbByFilter<ModelUser>(server, user, cb);
}

void GatewayMgr::onMysqlUpdateToDbORM(::pubsub::LOGIC_CMD& cmd)
{
	if (cmd.params_size() < 2)
	{
		return;
	}

	uint64_t userId = std::stoull(cmd.params()[0]);
	uint32_t level = std::stoul(cmd.params()[1]);


	ModelUser user;
	user.fields.user_id = userId;
	user.fields.level = level;
	bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());
	if (!bResult)
	{
		return;
	}
	user.markDirty({ 2 });

	::rpc_msg::CHANNEL server;
	server.set_type(common::EPT_DB_ROLE_Proxy);
	server.set_id(1);

	auto cb = [](rpc_msg::STATUS status, bool result, uint64_t affectedRows) {
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			return;
		}
	};
	UpdateToDb<ModelUser>(server, user, cb);
}

void GatewayMgr::onMysqlInsertToDbORM(::pubsub::LOGIC_CMD& cmd)
{
	if (cmd.params_size() < 2)
	{
		return;
	}

	uint64_t userId = std::stoull(cmd.params()[0]);
	uint32_t level = std::stoul(cmd.params()[1]);


	ModelUser user;
	user.fields.user_id = userId;
	user.fields.level = level;
	bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());
	if (!bResult)
	{
		return;
	}

	::rpc_msg::CHANNEL server;
	server.set_type(common::EPT_DB_ROLE_Proxy);
	server.set_id(1);

	auto cb = [](rpc_msg::STATUS status, bool result, uint64_t affectedRows, uint64_t insertId) {
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			return;
		}
	};
	InsertToDb<ModelUser>(server, user, cb);
}

void GatewayMgr::onMysqlDeleteFromDbORM(::pubsub::LOGIC_CMD& cmd)
{
	if (cmd.params_size() < 1)
	{
		return;
	}

	uint64_t userId = std::stoull(cmd.params()[0]);

	ModelUser user;
	user.fields.user_id = userId;
	bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());
	if (!bResult)
	{
		return;
	}

	::rpc_msg::CHANNEL server;
	server.set_type(common::EPT_DB_ROLE_Proxy);
	server.set_id(1);

	auto cb = [](rpc_msg::STATUS status, bool result, uint64_t affectedRows) {
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			return;
		}
	};
	DeleteFromDb<ModelUser>(server, user, cb);
}

void GatewayMgr::handleRequestClientLogin(uint64_t iSerialNum, const ::login_msg::MSG_REQUEST_CLIENT_LOGIN& request)
{
	ModelUser user;
	user.fields.user_id = request.user_id();

	bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());
	if (!bResult)
	{
		::login_msg::MSG_RESPONSE_CLIENT_LOGIN response;
		response.set_status_code(opcodes::SC_BindTable_Error);
		response.set_user_id(request.user_id());
		response.set_version(request.version());
		Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_MSG_RESPONSE_CLIENT_LOGIN, response);
		return;
	}

	::rpc_msg::CHANNEL server;
	server.set_type(common::EPT_DB_ROLE_Proxy);
	server.set_id(1);

	auto cb = [iSerialNum, request](rpc_msg::STATUS status, ModelUser user, uint32_t iRows) {
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			::login_msg::MSG_RESPONSE_CLIENT_LOGIN response;
			response.set_status_code(status.code());
			response.set_user_id(request.user_id());
			response.set_version(request.version());
			Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_MSG_RESPONSE_CLIENT_LOGIN, response);
			return;
		}

		::login_msg::MSG_RESPONSE_CLIENT_LOGIN response;
		response.set_status_code(status.code());
		response.set_user_id(request.user_id());
		response.set_version(request.version());
		if (iRows == 0)
		{
			response.set_is_newbie(true);
		} 
		else
		{
			response.set_is_newbie(false);
		}

		auto ptrGatewayRole = GatewayRole::createGatewayRole(user.fields.user_id, iSerialNum);
		GatewayMgrSingleton::get().addGatewayRole(ptrGatewayRole);

		Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_MSG_RESPONSE_CLIENT_LOGIN, response);
	};
	LoadFromDb<ModelUser>(server, user, cb);
}

void GatewayMgr::onServerPeerClose(uint64_t topic, ::google::protobuf::Message& msg)
{
	std::stringstream ss;

	auto& refMsg = dynamic_cast<::pubsub::SERVER_PEER_CLOSE&>(msg);
	ss << "topic:" << topic << ",refMsg:" << refMsg.ShortDebugString();
	ASYNC_PIE_LOG("GatewayMgr/onServerPeerClose", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	uint64_t iSerialNum = refMsg.serial_num();

	auto optRoleId = GatewayMgrSingleton::get().findRoleIdBySerialNum(iSerialNum);
	if (!optRoleId.has_value())
	{
		return;
	}

	GatewayMgrSingleton::get().removeGateWayRole(optRoleId.value());
}

}

