#include "service_init.h"

#include "apie.h"
#include "logic/dbproxy_mgr.h"

namespace APie {

std::tuple<uint32_t, std::string> initHook()
{
	DBProxyMgrSingleton::get().init();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> startHook()
{
	return DBProxyMgrSingleton::get().start();
}

std::tuple<uint32_t, std::string> exitHook()
{
	DBProxyMgrSingleton::get().exit();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

}

