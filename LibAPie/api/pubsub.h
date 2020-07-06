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

#include <google/protobuf/message.h>

#include "../singleton/threadsafe_singleton.h"

namespace APie {

class PubSub {
public:
	using SubscribeCallback = std::function<void(const std::string& topic, ::google::protobuf::Message& msg)>;

	struct SubEntry
	{
		uint64_t id = 0;
		SubscribeCallback cb;
		int32_t priority = 0;
	};

	using ChannelMap = std::map<std::string, std::vector<SubEntry>>;

	uint64_t subscribe(const std::string& topic, SubscribeCallback cb, int32_t priority = 0);
	void publish(const std::string& topic, ::google::protobuf::Message& msg);

private:
	uint64_t genarateId();

public:
	uint64_t m_id = { 0 };
	ChannelMap m_topicMap;

	//std::mutex m_topicMutex;
};

typedef ThreadSafeSingleton<PubSub> PubSubSingleton;



} 
