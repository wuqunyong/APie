#include "login_mgr.h"
#include "../../SharedDir/dao/model_account.h"

namespace APie {

std::tuple<uint32_t, std::string> LoginMgr::init()
{
	auto type = APie::CtxSingleton::get().getServerType();

	std::set<uint32_t> validType;
	validType.insert(common::EPT_Login_Server);

	if (validType.count(type) == 0)
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
	// 加载，数据表结构
	auto dbType = DeclarativeBase::DBType::DBT_Account;
	DAOFactoryTypeSingleton::get().registerRequiredTable(dbType, ModelAccount::getFactoryName(), ModelAccount::createMethod);

	auto requiredTableOpt = DAOFactoryTypeSingleton::get().getRequiredTable(dbType);
	if (!requiredTableOpt.has_value())
	{
		APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);

		return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
	}


	::rpc_msg::CHANNEL server;
	server.set_type(common::EPT_DB_ACCOUNT_Proxy);
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
	CallMysqlDescTable(server, DeclarativeBase::DBType::DBT_Account, tables, ptrReadyCb);

	return std::make_tuple(Hook::HookResult::HR_Ok, "HR_Ok");
}

std::tuple<uint32_t, std::string> LoginMgr::ready()
{
	// PubSub
	APie::PubSubSingleton::get().subscribe(::pubsub::PT_ServerPeerClose, LoginMgr::onServerPeerClose);


	// CLIENT OPCODE
	Api::PBHandler& serverPB = Api::OpcodeHandlerSingleton::get().server;
	serverPB.bind(::opcodes::OP_MSG_REQUEST_ACCOUNT_LOGIN_L, LoginMgr::handleAccountLogin, ::login_msg::MSG_REQUEST_ACCOUNT_LOGIN_L::default_instance());


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
	ASYNC_PIE_LOG("GatewayMgr/onServerPeerClose", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	uint64_t iSerialNum = refMsg.serial_num();

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
		Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, response);
		return;
	}

	::rpc_msg::CHANNEL server;
	server.set_type(common::EPT_DB_ACCOUNT_Proxy);
	server.set_id(1);

	auto cb = [iSerialNum, request, server](rpc_msg::STATUS status, ModelAccount account, uint32_t iRows) {
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			::login_msg::MSG_RESPONSE_ACCOUNT_LOGIN_L response;
			response.set_status_code(status.code());
			response.set_account_id(request.account_id());
			Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, response);
			return;
		}

		::login_msg::MSG_RESPONSE_ACCOUNT_LOGIN_L response;
		response.set_status_code(status.code());
		response.set_account_id(request.account_id());
		if (iRows != 0)
		{
			Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_MSG_RESPONSE_CLIENT_LOGIN, response);
			return;
		}

		auto curTime = time(NULL);
		account.fields.register_time = curTime;
		account.fields.modified_time = curTime;

		auto cb = [iSerialNum, response](rpc_msg::STATUS status, bool result, uint64_t affectedRows, uint64_t insertId) mutable {
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				response.set_status_code(status.code());
				Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_MSG_RESPONSE_CLIENT_LOGIN, response);
				return;
			}

			Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_MSG_RESPONSE_CLIENT_LOGIN, response);
		};
		InsertToDb<ModelAccount>(server, account, cb);
	};
	LoadFromDb<ModelAccount>(server, accountData, cb);
}

}

