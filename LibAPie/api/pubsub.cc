#include "pubsub.h"

#include <chrono>
#include <string>
#include <iosfwd>
#include <sstream>
#include <algorithm> 

#include "../network/logger.h"


namespace APie {

	uint64_t PubSub::subscribe(uint64_t topic, SubscribeCallback cb, int32_t priority)
	{
		uint64_t id = genarateId();
		this->subscribeImpl(topic, id, cb, priority);
		return id;
	}

	void PubSub::subscribeImpl(uint64_t topic, uint64_t id, SubscribeCallback cb, int32_t priority)
	{
		SubEntry entry;
		entry.id = id;
		entry.cb = cb;
		entry.priority = priority;

		auto findIte = m_topicMap.find(topic);
		if (findIte == m_topicMap.end())
		{
			std::vector<SubEntry> cbVec;
			cbVec.push_back(entry);
			m_topicMap[topic] = cbVec;
		}
		else
		{
			findIte->second.push_back(entry);
		}
	}

	void PubSub::unregister(uint64_t topic, uint64_t id)
	{
		auto findIte = m_topicMap.find(topic);
		if (findIte == m_topicMap.end())
		{
			return;
		}

		auto cmp = [id](SubEntry entry) {
			if (entry.id == id)
			{
				return true;
			}

			return false;
		};
		findIte->second.erase(std::remove_if(findIte->second.begin(), findIte->second.end(), cmp), findIte->second.end());
	}

	void PubSub::publish(uint64_t topic, ::google::protobuf::Message& msg)
	{
		auto findIte = m_topicMap.find(topic);
		if (findIte != m_topicMap.end())
		{
			auto ptrCmp = [](SubEntry& lhs, SubEntry& rhs) {
				return lhs.priority < rhs.priority;
			};
			std::sort(findIte->second.begin(), findIte->second.end(), ptrCmp);

			for (auto& item : findIte->second)
			{
				try
				{
					item.cb(topic, msg);
				}
				catch (const std::bad_cast& e)
				{
					std::stringstream ss;
					ss << "publish error: " << e.what();
					ASYNC_PIE_LOG("bad_cast:%s", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());
				}
				catch (const std::exception& e)
				{
					std::stringstream ss;
					ss << "publish error: " << e.what();
					ASYNC_PIE_LOG("exception:%s", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());
				}
			}
		}
	}

	uint64_t PubSub::genarateId()
	{
		//std::lock_guard<std::mutex> lock(m_topicMutex);
		++m_id;

		return m_id;
	}

} // namespace APie
