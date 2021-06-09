#include "service_init.h"

#include "apie.h"
#include "logic/login_mgr.h"

namespace APie {

std::tuple<uint32_t, std::string> initHook()
{
	return LoginMgrSingleton::get().init();
}

std::tuple<uint32_t, std::string> startHook()
{
	return LoginMgrSingleton::get().start();
}

std::tuple<uint32_t, std::string> readyHook()
{
	return LoginMgrSingleton::get().ready();
}

std::tuple<uint32_t, std::string> exitHook()
{
	LoginMgrSingleton::get().exit();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

}

