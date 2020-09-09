#include "service_init.h"

#include "apie.h"
#include "logic/login_mgr.h"

namespace APie {

std::tuple<uint32_t, std::string> initHook()
{
	LoginMgrSingleton::get().init();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> startHook()
{
	LoginMgrSingleton::get().start();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> exitHook()
{
	LoginMgrSingleton::get().exit();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

}

