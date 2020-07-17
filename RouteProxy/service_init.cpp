#include "service_init.h"

#include "apie.h"
#include "logic/route_proxy.h"

namespace APie {

std::tuple<uint32_t, std::string> initHook()
{
	RouteProxySingleton::get().init();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> startHook()
{
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> exitHook()
{
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

}

