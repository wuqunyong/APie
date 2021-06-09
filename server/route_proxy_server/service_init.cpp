#include "service_init.h"

#include "apie.h"
#include "logic/route_proxy.h"

namespace APie {

std::tuple<uint32_t, std::string> initHook()
{
	return RouteProxySingleton::get().init();
}

std::tuple<uint32_t, std::string> startHook()
{
	return RouteProxySingleton::get().start();
}

std::tuple<uint32_t, std::string> readyHook()
{
	return RouteProxySingleton::get().ready();
}

std::tuple<uint32_t, std::string> exitHook()
{
	RouteProxySingleton::get().exit();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

}

