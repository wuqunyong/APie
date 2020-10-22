#include "service_init.h"

#include "apie.h"
#include "logic/test_server.h"

namespace APie {

std::tuple<uint32_t, std::string> initHook()
{
	return TestServerMgrSingleton::get().init();
}

std::tuple<uint32_t, std::string> startHook()
{
	return TestServerMgrSingleton::get().start();
}

std::tuple<uint32_t, std::string> readyHook()
{
	return TestServerMgrSingleton::get().ready();
}

std::tuple<uint32_t, std::string> exitHook()
{
	TestServerMgrSingleton::get().exit();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

}

