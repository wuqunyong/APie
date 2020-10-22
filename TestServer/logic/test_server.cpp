#include "test_server.h"

namespace APie {


std::tuple<uint32_t, std::string> TestServerMgr::init()
{
	APie::RPC::rpcInit();



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
}

