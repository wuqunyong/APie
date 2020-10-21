#include "login_mgr.h"

namespace APie {

std::tuple<uint32_t, std::string> LoginMgr::init()
{
	APie::RPC::rpcInit();

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> LoginMgr::start()
{
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> LoginMgr::ready()
{
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

void LoginMgr::exit()
{

}

}

