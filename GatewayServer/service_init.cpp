#include "service_init.h"

#include "apie.h"
#include "logic/gateway_mgr.h"

namespace APie {

std::tuple<uint32_t, std::string> initHook()
{
	return GatewayMgrSingleton::get().init();
}

std::tuple<uint32_t, std::string> startHook()
{
	return GatewayMgrSingleton::get().start();
}

std::tuple<uint32_t, std::string> readyHook()
{
	return GatewayMgrSingleton::get().ready();
}

std::tuple<uint32_t, std::string> exitHook()
{
	GatewayMgrSingleton::get().exit();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

}

