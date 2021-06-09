#include "service_init.h"

#include "apie.h"
#include "logic/service_registry.h"

namespace APie {

std::tuple<uint32_t, std::string> initHook()
{
	return ServiceRegistrySingleton::get().init();
}

std::tuple<uint32_t, std::string> startHook()
{
	return ServiceRegistrySingleton::get().start();
}

std::tuple<uint32_t, std::string> readyHook()
{
	return ServiceRegistrySingleton::get().ready();
}

std::tuple<uint32_t, std::string> exitHook()
{
	ServiceRegistrySingleton::get().exit();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

}

