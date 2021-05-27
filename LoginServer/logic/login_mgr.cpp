#include "login_mgr.h"

#include "../../LibAPie/common/string_utils.h"
#include "../../SharedDir/dao/model_account.h"
#include "../../SharedDir/opcodes.h"


namespace APie {

std::tuple<uint32_t, std::string> LoginMgr::init()
{
	auto bResult = APie::CtxSingleton::get().checkIsValidServerType({ common::EPT_Login_Server });
	if (!bResult)
	{
		return std::make_tuple(Hook::HookResult::HR_Error, "invalid Type");
	}

	// CMD
	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_LogicCmd, LoginMgr::onLogicCommnad);

	LogicCmdHandlerSingleton::get().init();

	// RPC
	APie::RPC::rpcInit();

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> LoginMgr::start()
{
	// 加载:数据表结构
	auto dbType = DeclarativeBase::DBType::DBT_Account;
	auto ptrReadyCb = [](bool bResul, std::string sInfo, uint64_t iCallCount) {
		if (!bResul)
		{
			std::stringstream ss;
			ss << "CallMysqlDescTable|bResul:" << bResul << ",sInfo:" << sInfo << ",iCallCount:" << iCallCount;

			PANIC_ABORT(ss.str().c_str());
		}

		APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);

	};

	bool bResult = RegisterRequiredTable(dbType, 1, { {ModelAccount::getFactoryName(), ModelAccount::createMethod} }, ptrReadyCb);
	if (bResult)
	{
		return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
	}
	else
	{
		return std::make_tuple(Hook::HookResult::HR_Error, "HR_Error");
	}
}

std::tuple<uint32_t, std::string> LoginMgr::ready()
{
	// PubSub
	APie::PubSubSingleton::get().subscribe(::pubsub::PT_ServerPeerClose, LoginMgr::onServerPeerClose);


	// CLIENT OPCODE
	Api::PBHandler& serverPB = Api::OpcodeHandlerSingleton::get().server;
	serverPB.bind(::APie::OP_MSG_REQUEST_ACCOUNT_LOGIN_L, LoginMgr::handleAccountLogin);


	std::stringstream ss;
	ss << "Server Ready!";
	std::cout << ss.str() << std::endl;
	ASYNC_PIE_LOG("ServerStatus", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

void LoginMgr::exit()
{

}

void LoginMgr::onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg)
{
	auto& command = dynamic_cast<::pubsub::LOGIC_CMD&>(msg);
	auto handlerOpt = LogicCmdHandlerSingleton::get().findCb(command.cmd());
	if (!handlerOpt.has_value())
	{
		return;
	}

	handlerOpt.value()(command);
}

void LoginMgr::onServerPeerClose(uint64_t topic, ::google::protobuf::Message& msg)
{
	std::stringstream ss;

	auto& refMsg = dynamic_cast<::pubsub::SERVER_PEER_CLOSE&>(msg);
	ss << "topic:" << topic << ",refMsg:" << refMsg.ShortDebugString();
	ASYNC_PIE_LOG("LoginMgr/onServerPeerClose", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	//uint64_t iSerialNum = refMsg.serial_num();

}

void LoginMgr::handleAccountLogin(uint64_t iSerialNum, const ::login_msg::MSG_REQUEST_ACCOUNT_LOGIN_L& request)
{
	ModelAccount accountData;
	accountData.fields.account_id = request.account_id();

	bool bResult = accountData.bindTable(DeclarativeBase::DBType::DBT_Account, ModelAccount::getFactoryName());
	if (!bResult)
	{
		::login_msg::MSG_RESPONSE_ACCOUNT_LOGIN_L response;
		response.set_status_code(opcodes::SC_BindTable_Error);
		response.set_account_id(request.account_id());
		Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, response);
		return;
	}

	::rpc_msg::CHANNEL server;
	server.set_type(common::EPT_DB_ACCOUNT_Proxy);
	server.set_id(1);


	// 测试
	auto multiCb = [](const rpc_msg::STATUS& status, std::tuple<ModelAccount, ModelAccount, ModelAccount>& tupleData, std::array<uint32_t, 3>& tupleRows) {
		int a = 1;
		int c = a + 1;
	};
	bResult = Multi_LoadFromDb(multiCb, server, accountData, accountData, accountData);


	auto cb = [iSerialNum, request, server](rpc_msg::STATUS status, ModelAccount account, uint32_t iRows) {
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			::login_msg::MSG_RESPONSE_ACCOUNT_LOGIN_L response;
			response.set_status_code(status.code());
			response.set_account_id(request.account_id());
			Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, response);
			return;
		}

		::login_msg::MSG_RESPONSE_ACCOUNT_LOGIN_L response;
		response.set_status_code(status.code());
		response.set_account_id(request.account_id());

		auto gatewayOpt = EndPointMgrSingleton::get().modulusEndpointById(::common::EPT_Gateway_Server, request.account_id());
		if (!gatewayOpt.has_value())
		{
			response.set_status_code(opcodes::SC_Discovery_ServerListEmpty);
			Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, response);
			return;
		}

		std::string ip = gatewayOpt.value().ip();
		uint32_t port = gatewayOpt.value().port();
		std::string sessionKey = APie::randomStr(16);

		response.set_ip(ip);
		response.set_port(port);
		response.set_session_key(sessionKey);

		::rpc_login::L2G_LoginPendingRequest rpcRequest;
		rpcRequest.set_account_id(request.account_id());
		rpcRequest.set_session_key(sessionKey);
		rpcRequest.set_db_id(account.fields.db_id);
		rpcRequest.set_version(request.version());

		if (iRows != 0)
		{
			auto iAccountId = request.account_id();

			auto curTime = time(NULL);
			account.fields.modified_time = curTime;

			account.markDirty({ ModelAccount::modified_time });
			auto cb = [iAccountId](rpc_msg::STATUS status, bool result, uint64_t affectedRows) {
				if (status.code() != ::rpc_msg::CODE_Ok)
				{
					std::stringstream ss;
					ss << "UpdateToDb Error|accountId:" << iAccountId;
					ASYNC_PIE_LOG("LoginMgr/handleAccountLogin", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());
					return;
				}
			};
			UpdateToDb<ModelAccount>(server, account, cb);

			::rpc_msg::CHANNEL server;
			server.set_type(gatewayOpt.value().type());
			server.set_id(gatewayOpt.value().id());

			auto rpcCB = [iSerialNum, response](const rpc_msg::STATUS& status, const std::string& replyData) mutable
			{
				if (status.code() != ::rpc_msg::CODE_Ok)
				{
					response.set_status_code(status.code());
					Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, response);
					return;
				}

				Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, response);
			};
			APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_L2G_LoginPending, rpcRequest, rpcCB);
			return;
		}

		auto roleDBopt = EndPointMgrSingleton::get().modulusEndpointById(::common::EPT_DB_ROLE_Proxy, request.account_id());
		if (!roleDBopt.has_value())
		{
			response.set_status_code(opcodes::SC_Discovery_ServerListEmpty);
			Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, response);
			return;
		}


		auto curTime = time(NULL);
		account.fields.register_time = curTime;
		account.fields.modified_time = curTime;
		account.fields.db_id = roleDBopt.value().id();

		auto cb = [iSerialNum, response, gatewayOpt, rpcRequest](rpc_msg::STATUS status, bool result, uint64_t affectedRows, uint64_t insertId) mutable {
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				response.set_status_code(status.code());
				Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, response);
				return;
			}

			::rpc_msg::CHANNEL server;
			server.set_type(gatewayOpt.value().type());
			server.set_id(gatewayOpt.value().id());

			auto rpcCB = [iSerialNum, response](const rpc_msg::STATUS& status, const std::string& replyData) mutable
			{
				if (status.code() != ::rpc_msg::CODE_Ok)
				{
					response.set_status_code(status.code());
					Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, response);
					return;
				}

				Network::OutputStream::sendMsg(iSerialNum, APie::OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, response);
			};
			APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_L2G_LoginPending, rpcRequest, rpcCB);
		};
		InsertToDb<ModelAccount>(server, account, cb);
	};
	LoadFromDb<ModelAccount>(server, accountData, cb);
}

}

