#include "login_mgr.h"

namespace APie {

std::tuple<uint32_t, std::string> LoginMgr::init()
{
	APie::RPC::rpcInit();

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> LoginMgr::start()
{
	APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> LoginMgr::ready()
{
	std::stringstream ss;
	ss << "Server Ready!";
	std::cout << ss.str() << std::endl;
	ASYNC_PIE_LOG("ServerStatus", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

void LoginMgr::exit()
{

}

}

