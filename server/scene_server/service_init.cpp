#include "service_init.h"

#include "apie.h"
#include "logic/scene_mgr.h"

namespace APie {

std::tuple<uint32_t, std::string> initHook()
{
	return SceneMgrSingleton::get().init();
}

std::tuple<uint32_t, std::string> startHook()
{
	return SceneMgrSingleton::get().start();
}

std::tuple<uint32_t, std::string> readyHook()
{
	return SceneMgrSingleton::get().ready();
}

std::tuple<uint32_t, std::string> exitHook()
{
	SceneMgrSingleton::get().exit();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

}

