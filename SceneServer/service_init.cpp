#include "service_init.h"

#include "apie.h"
#include "logic/scene_mgr.h"

namespace APie {

std::tuple<uint32_t, std::string> initHook()
{
	SceneMgrSingleton::get().init();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> startHook()
{
	SceneMgrSingleton::get().start();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> exitHook()
{
	SceneMgrSingleton::get().exit();
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

}

