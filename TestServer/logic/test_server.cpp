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
	uint32_t maskFlag = APie::CtxSingleton::get().yamlAs<uint16_t>({ "clients", "socket_address", "mask_flag" }, 0);

	m_ptrClientProxy = APie::ClientProxy::createClientProxy();
	auto connectCb = [](APie::ClientProxy* ptrClient, uint32_t iResult) {
		if (iResult == 0)
		{
			APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);
		}
		return true;
	};
	m_ptrClientProxy->connect(ip, port, static_cast<APie::ProtocolType>(type), maskFlag, connectCb);
	m_ptrClientProxy->addReconnectTimer(10000000);

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

void TestServerMgr::addMockRole(std::shared_ptr<MockRole> ptrMockRole)
{
	auto iRoleId = ptrMockRole->getRoleId();
	m_mockRole[iRoleId] = ptrMockRole;
}

std::shared_ptr<MockRole> TestServerMgr::findMockRole(uint64_t iRoleId)
{
	auto findIte = m_mockRole.find(iRoleId);
	if (findIte == m_mockRole.end())
	{
		return nullptr;
	}

	return findIte->second;
}

void TestServerMgr::removeMockRole(uint64_t iRoleId)
{
	m_mockRole.erase(iRoleId);
}

void TestServerMgr::addSerialNumRole(uint64_t iSerialNum, uint64_t iRoleId)
{
	m_serialNumRole[iSerialNum] = iRoleId;
}

std::optional<uint64_t> TestServerMgr::findRoleIdBySerialNum(uint64_t iSerialNum)
{
	auto findIte = m_serialNumRole.find(iSerialNum);
	if (findIte == m_serialNumRole.end())
	{
		return std::nullopt;
	}

	return findIte->second;
}

void TestServerMgr::removeSerialNum(uint64_t iSerialNum)
{
	m_serialNumRole.erase(iSerialNum);
}

void TestServerMgr::onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg)
{
	auto& command = dynamic_cast<::pubsub::LOGIC_CMD&>(msg);

	if (command.cmd() == "login")
	{
		if (command.params_size() < 2)
		{
			return;
		}

		::login_msg::MSG_REQUEST_CLIENT_LOGIN request;
		request.set_user_id(std::stoull(command.params()[0]));
		request.set_version(std::stoi(command.params()[1]));
		request.set_session_key(command.params()[2]);

		TestServerMgrSingleton::get().m_ptrClientProxy->sendMsg(::opcodes::OP_MSG_REQUEST_CLIENT_LOGIN, request);

		std::cout << "send|iSerialNum:" << TestServerMgrSingleton::get().m_ptrClientProxy->getSerialNum() << "|request:" << request.ShortDebugString() << std::endl;
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

		TestServerMgrSingleton::get().m_ptrClientProxy->sendMsg(::opcodes::OP_MSG_REQUEST_ECHO, request);

		std::cout << "send|iSerialNum:" << TestServerMgrSingleton::get().m_ptrClientProxy->getSerialNum() << "|request:" << request.ShortDebugString() << std::endl;
	}
	else if (command.cmd() == "client")
	{
		if (command.params_size() < 3)
		{
			return;
		}

		uint64_t iRoleId = std::stoull(command.params()[0]);
		auto ptrMockRole = TestServerMgrSingleton::get().findMockRole(iRoleId);
		if (ptrMockRole == nullptr)
		{
			ptrMockRole = MockRole::createMockRole(iRoleId);
			ptrMockRole->start();

			TestServerMgrSingleton::get().addMockRole(ptrMockRole);
		}

		::pubsub::LOGIC_CMD newMsg;
		for (int i = 1; i < command.params().size(); i++)
		{
			if (i == 1)
			{
				newMsg.set_cmd(command.params()[i]);
			}
			else
			{
				auto ptrAdd = newMsg.add_params();
				*ptrAdd = command.params()[i];
			}
		}
		//ptrMockRole->clearMsg();
		ptrMockRole->pushMsg(newMsg);
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

