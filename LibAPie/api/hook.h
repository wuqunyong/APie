#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <tuple>
#include <mutex>
#include <optional>
#include "../singleton/threadsafe_singleton.h"


namespace APie {
namespace Hook {

enum class HookPoint
{
	HP_Init = 0,
	HP_Start = 1,
	HP_Exit = 2,
};

enum HookResult
{
	HR_Ok = 0,
	HR_Error = 1,
};

class HookRegistry {
public:
	using HookCallback = std::function<std::tuple<uint32_t, std::string>(void)>;

	struct HookEntry
	{
		HookCallback cb;
		uint32_t priority = 0;
	};

	using HookCallbackMap = std::map<HookPoint, std::vector<HookEntry>>;

	void appendHook(HookPoint point, HookCallback cb, uint32_t priority = 0);
	std::optional<std::vector<HookEntry>> getHook(HookPoint point);

	void triggerHook(HookPoint point);

public:
	HookCallbackMap m_hookMap;
	std::mutex m_hookMutex;
};

typedef ThreadSafeSingleton<HookRegistry> HookRegistrySingleton;

} 
} 
