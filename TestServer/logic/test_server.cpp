#include "test_server.h"
#include "../../PBMsg/login_msg.pb.h"

namespace APie {


std::tuple<uint32_t, std::string> TestServerMgr::init()
{
	APie::RPC::rpcInit();

	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_LogicCmd, TestServerMgr::onLogicCommnad);
	
	Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OP_MSG_RESPONSE_CLIENT_LOGIN, TestServerMgr::handleResponseClientLogin, ::login_msg::MSG_RESPONSE_CLIENT_LOGIN::default_instance());
	Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OP_MSG_RESPONSE_ECHO, TestServerMgr::handleResponseEcho, ::login_msg::MSG_RESPONSE_ECHO::default_instance());


	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> TestServerMgr::start()
{
	std::string ip = APie::CtxSingleton::get().yamlAs<std::string>({ "clients", "socket_address", "address" }, "");
	uint16_t port = APie::CtxSingleton::get().yamlAs<uint16_t>({ "clients", "socket_address", "port_value" }, 0);
	uint16_t type = APie::CtxSingleton::get().yamlAs<uint16_t>({ "clients", "socket_address", "type" }, 0);

	m_ptrClientProxy = APie::ClientProxy::createClientProxy();
	auto connectCb = [](APie::ClientProxy* ptrClient, uint32_t iResult) {
		if (iResult == 0)
		{
			APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);
		}
		return true;
	};
	m_ptrClientProxy->connect(ip, port, static_cast<APie::ProtocolType>(type), connectCb);
	m_ptrClientProxy->addReconnectTimer(1000);

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> TestServerMgr::ready()
{
	std::stringstream ss;
	ss << "Server Ready!" << std::endl;
	std::cout << ss.str();
	ASYNC_PIE_LOG("ServerStatus", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

void TestServerMgr::exit()
{
}

void TestServerMgr::onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg)
{
	auto& command = dynamic_cast<::pubsub::LOGIC_CMD&>(msg);

	if (command.cmd() == "login")
	{
		if (command.params_size() < 3)
		{
			return;
		}

		::login_msg::MSG_REQUEST_CLIENT_LOGIN request;
		request.set_user_id(std::stoull(command.params()[0]));
		request.set_version(std::stoi(command.params()[1]));
		request.set_session_key(command.params()[2]);

		auto iSerialNum = TestServerMgrSingleton::get().m_ptrClientProxy->getSerialNum();
		Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OP_MSG_REQUEST_CLIENT_LOGIN, request, APie::ConnetionType::CT_CLIENT);

		std::cout << "send|iSerialNum:" << iSerialNum << "|request:" << request.ShortDebugString() << std::endl;
	}
	else if (command.cmd() == "echo")
	{
		if (command.params_size() < 1)
		{
			return;
		}

		uint64_t iCurMS = Ctx::getNowMilliseconds();

		::login_msg::MSG_REQUEST_ECHO request;
		request.set_value1(iCurMS);
		request.set_value2(command.params()[0]);

		auto iSerialNum = TestServerMgrSingleton::get().m_ptrClientProxy->getSerialNum();
		Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OP_MSG_REQUEST_ECHO, request, APie::ConnetionType::CT_CLIENT);

		std::cout << "send|iSerialNum:" << iSerialNum << "|request:" << request.ShortDebugString() << std::endl;
	}

}

void TestServerMgr::handleResponseClientLogin(uint64_t iSerialNum, const ::login_msg::MSG_RESPONSE_CLIENT_LOGIN& response)
{
	std::cout << "recv|iSerialNum:" << iSerialNum << "|response:" << response.ShortDebugString() << std::endl;
}

void TestServerMgr::handleResponseEcho(uint64_t iSerialNum, const ::login_msg::MSG_RESPONSE_ECHO& response)
{
	std::cout << "recv|iSerialNum:" << iSerialNum << "|response:" << response.ShortDebugString() << std::endl;
}

}

