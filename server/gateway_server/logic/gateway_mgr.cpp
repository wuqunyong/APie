#include "gateway_mgr.h"

#include "../../common/dao/model_user.h"
#include "../../common/opcodes.h"

#include "gateway_role.h"

#include "../../libapie/common/message_traits.h"
#include "../../libapie/common/file.h"


namespace APie {


std::tuple<uint32_t, std::string> GatewayMgr::init()
{
	auto bResult = APie::CtxSingleton::get().checkIsValidServerType({ common::EPT_Gateway_Server });
	if (!bResult)
	{
		return std::make_tuple(Hook::HookResult::HR_Error, "invalid Type");
	}

	std::string pubKey = APie::CtxSingleton::get().yamlAs<std::string>({ "certificate", "public_key" }, "/usr/local/apie/etc/key.pub");
	std::string privateKey = APie::CtxSingleton::get().yamlAs<std::string>({ "certificate", "private_key" }, "/usr/local/apie/etc/key.pem");

	std::string errInfo;
	bResult = APie::Crypto::RSAUtilitySingleton::get().init(pubKey, privateKey, errInfo);
	if (!bResult)
	{
		return std::make_tuple(Hook::HookResult::HR_Error, errInfo);
	}

	// CMD
	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_LogicCmd, GatewayMgr::onLogicCommnad);

	LogicCmdHandlerSingleton::get().init();
	LogicCmdHandlerSingleton::get().registerOnCmd("insert_to_db", "mysql_insert_to_db_orm", GatewayMgr::onMysqlInsertToDbORM);
	LogicCmdHandlerSingleton::get().registerOnCmd("delete_from_db", "mysql_delete_from_db_orm", GatewayMgr::onMysqlDeleteFromDbORM);
	LogicCmdHandlerSingleton::get().registerOnCmd("update_to_db", "mysql_update_to_db_orm", GatewayMgr::onMysqlUpdateToDbORM);
	LogicCmdHandlerSingleton::get().registerOnCmd("load_from_db", "mysql_load_from_db_orm", GatewayMgr::onMysqlLoadFromDbORM);
	LogicCmdHandlerSingleton::get().registerOnCmd("query_from_db", "mysql_query_from_db_orm", GatewayMgr::onMysqlQueryFromDbORM);
	
	LogicCmdHandlerSingleton::get().registerOnCmd("nats_publish", "nats_publish", GatewayMgr::onNatsPublish);

	// RPC
	APie::RPC::rpcInit();
	APie::RPC::RpcServerSingleton::get().bind(rpc_msg::RPC_DeMultiplexer_Forward, GatewayMgr::RPC_handleDeMultiplexerForward);
	APie::RPC::RpcServerSingleton::get().bind(rpc_msg::RPC_L2G_LoginPending, GatewayMgr::RPC_handleLoginPending);

	return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
}


std::tuple<uint32_t, std::string> GatewayMgr::start()
{
	// 加载:数据表结构
	auto dbType = DeclarativeBase::DBType::DBT_Role;
	auto ptrReadyCb = [](bool bResul, std::string sInfo, uint64_t iCallCount) {
		if (!bResul)
		{
			std::stringstream ss;
			ss << "CallMysqlDescTable|bResul:" << bResul << ",sInfo:" << sInfo << ",iCallCount:" << iCallCount;

			PANIC_ABORT(ss.str().c_str());
		}

		APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);

	};

	bool bResult = RegisterRequiredTable(dbType, 1, { {ModelUser::getFactoryName(), ModelUser::createMethod} }, ptrReadyCb);
	if (bResult)
	{
		return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
	}
	else
	{
		return std::make_tuple(Hook::HookResult::HR_Error, "HR_Error");
	}
}

std::tuple<uint32_t, std::string> GatewayMgr::ready()
{
	// PubSub
	APie::PubSubSingleton::get().subscribe(::pubsub::PT_ServerPeerClose, GatewayMgr::onServerPeerClose);


	// CLIENT OPCODE
	Api::PBHandler& serverPB = Api::OpcodeHandlerSingleton::get().server;
	serverPB.setDefaultFunc(GatewayMgr::handleDefaultOpcodes);
	serverPB.bind(::APie::OP_MSG_REQUEST_CLIENT_LOGIN, GatewayMgr::handleRequestClientLogin);
	serverPB.bind(::APie::OP_MSG_REQUEST_HANDSHAKE_INIT, GatewayMgr::handleRequestHandshakeInit);
	serverPB.bind(::APie::OP_MSG_REQUEST_HANDSHAKE_ESTABLISHED, GatewayMgr::handleRequestHandshakeEstablished);
	
	//static_assert(std::is_function<decltype(GatewayMgr::handleRequestHandshakeEstablished)>::value);

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

void GatewayMgr::addPendingRole(const PendingLoginRole &role)
{
	m_pendingRole[role.role_id] = role;
}

std::optional<PendingLoginRole> GatewayMgr::getPendingRole(uint64_t iRoleId)
{
	auto findIte = m_pendingRole.find(iRoleId);
	if (findIte == m_pendingRole.end())
	{
		return std::nullopt;
	}

	return findIte->second;
}

void GatewayMgr::removePendingRole(uint64_t iRoleId)
{
	m_pendingRole.erase(iRoleId);
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

	bool bResult = ptrGatewayRole->addRequestPerUnit(1);
	if (!bResult)
	{
		return;
	}

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

std::tuple<uint32_t, std::string> GatewayMgr::RPC_handleLoginPending(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::rpc_login::L2G_LoginPendingRequest& request)
{
	auto curTime = time(nullptr);

	PendingLoginRole role;
	role.role_id = request.account_id();
	role.session_key = request.session_key();
	role.db_id = request.db_id();
	role.expires_at = curTime + 30;

	GatewayMgrSingleton::get().addPendingRole(role);

	::rpc_login::L2G_LoginPendingResponse response;
	response.set_status_code(opcodes::SC_Ok);
	response.set_account_id(request.account_id());

	return std::make_tuple(::rpc_msg::CODE_Ok, response.SerializeAsString());
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

void GatewayMgr::onNatsPublish(::pubsub::LOGIC_CMD& cmd)
{
	if (cmd.params_size() < 4)
	{
		return;
	}

	uint32_t type = std::stoul(cmd.params()[0]);
	uint32_t id = std::stoul(cmd.params()[1]);

	std::string channel = APie::Event::NatsManager::GetTopicChannel(type, id);

	std::string name = cmd.params()[2];
	std::string info = cmd.params()[3];

	::nats_msg::NATS_MSG_PRXOY nats_msg;
	APie::Event::NatsSingleton::get().publishNatsMsg(APie::Event::NatsManager::E_NT_Realm, channel, nats_msg);

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
		Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_CLIENT_LOGIN, response);
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
			Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_CLIENT_LOGIN, response);
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

		Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_CLIENT_LOGIN, response);
	};
	LoadFromDb<ModelUser>(server, user, cb);
}

void GatewayMgr::handleRequestHandshakeInit(uint64_t iSerialNum, const ::login_msg::MSG_REQUEST_HANDSHAKE_INIT& request)
{
	std::string content;
	std::string pubKey = APie::CtxSingleton::get().yamlAs<std::string>({ "certificate", "public_key" }, "/usr/local/apie/etc/key.pub");
	bool bResult = APie::Common::GetContent(pubKey, &content);

	std::string sServerRandom("server");

	::login_msg::MSG_RESPONSE_HANDSHAKE_INIT response;

	if (bResult)
	{
		response.set_status_code(opcodes::SC_Ok);
	}
	else
	{
		response.set_status_code(opcodes::SC_Auth_LoadPubFileError);
	}
	response.set_server_random(sServerRandom);
	response.set_public_key(content);
	Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_HANDSHAKE_INIT, response);

	APie::SetServerSessionAttr *ptr = new APie::SetServerSessionAttr;
	ptr->iSerialNum = iSerialNum;
	ptr->optClientRandom = request.client_random();
	ptr->optServerRandom = sServerRandom;

	Command cmd;
	cmd.type = Command::set_server_session_attr;
	cmd.args.set_server_session_attr.ptrData = ptr;
	Network::OutputStream::sendCommand(ConnetionType::CT_SERVER, iSerialNum, cmd);
}

void GatewayMgr::handleRequestHandshakeEstablished(uint64_t iSerialNum, const ::login_msg::MSG_REQUEST_HANDSHAKE_ESTABLISHED& request)
{
	std::string decryptedMsg;
	bool bResult = APie::Crypto::RSAUtilitySingleton::get().decrypt(request.encrypted_key(), &decryptedMsg);
	if (!bResult)
	{
		::login_msg::MSG_RESPONSE_HANDSHAKE_ESTABLISHED response;
		response.set_status_code(opcodes::SC_Auth_DecryptError);
		Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_HANDSHAKE_ESTABLISHED, response);
		return;
	}

	auto ptrConnection = Event::DispatcherImpl::getConnection(iSerialNum);
	if (ptrConnection == nullptr)
	{
		return;
	}

	std::string sClientRandom = ptrConnection->getClientRandom();
	std::string sServerRandom = ptrConnection->getServerRandom();

	std::string sSessionKey = sClientRandom + sServerRandom + decryptedMsg;

	::login_msg::MSG_RESPONSE_HANDSHAKE_ESTABLISHED response;
	response.set_status_code(opcodes::SC_Ok);
	Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_HANDSHAKE_ESTABLISHED, response);

	APie::SetServerSessionAttr *ptr = new APie::SetServerSessionAttr;
	ptr->iSerialNum = iSerialNum;
	ptr->optKey = sSessionKey;

	Command cmd;
	cmd.type = Command::set_server_session_attr;
	cmd.args.set_server_session_attr.ptrData = ptr;
	Network::OutputStream::sendCommand(ConnetionType::CT_SERVER, iSerialNum, cmd);
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

