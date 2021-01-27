#include "hook.h"

#include <chrono>
#include <string>
#include <iosfwd>
#include <sstream>
#include <algorithm> 

#include "../network/logger.h"


namespace APie {
namespace Hook {

	void HookRegistry::registerHook(HookPoint point, HookCallback cb, uint32_t priority)
	{
		HookEntry entry;
		entry.cb = cb;
		entry.priority = priority;

		auto findIte = m_hookMap.find(point);
		if (findIte == m_hookMap.end())
		{
			std::vector<HookEntry> cbVec;
			cbVec.push_back(entry);
			m_hookMap[point] = cbVec;
		}
		else
		{
			findIte->second.push_back(entry);
		}
	}


	std::optional<std::vector<HookRegistry::HookEntry>> HookRegistry::getHook(HookPoint point)
	{
		auto findIte = m_hookMap.find(point);
		if (findIte == m_hookMap.end())
		{
			return std::nullopt;
		}

		return std::make_optional(findIte->second);
	}

	void HookRegistry::triggerHook(HookPoint point)
	{
		auto initCbOpt = APie::Hook::HookRegistrySingleton::get().getHook(point);
		if (initCbOpt.has_value())
		{
			auto ptrCmp = [](HookEntry& lhs, HookEntry& rhs){
				return lhs.priority < rhs.priority;
			};
			std::sort(initCbOpt.value().begin(), initCbOpt.value().end(), ptrCmp);

			for (auto& item : initCbOpt.value())
			{
				auto result = item.cb();
				if (std::get<0>(result) != 0)
				{
					std::stringstream ss;
					ss << "errorCode:" << std::get<0>(result) << "|info:" << std::get<1>(result);

					if (point == HookPoint::HP_Exit)
					{
						PIE_LOG("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "exit|%s", ss.str().c_str());
						continue;
					}

					PANIC_ABORT(ss.str().c_str());
				}
			}
		}
	}

} // namespace Hook
} // namespace APie
