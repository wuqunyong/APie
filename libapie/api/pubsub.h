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

#include "../pb_msg.h"


namespace APie {


class PubSub {
public:

	using SubscribeCallback = std::function<void(uint64_t topic, ::google::protobuf::Message& msg)>;

	struct SubEntry
	{
		uint64_t id = 0;
		SubscribeCallback cb;
		int32_t priority = 0;
	};

	using ChannelMap = std::map<uint64_t, std::vector<SubEntry>>;

	uint64_t subscribe(uint64_t topic, SubscribeCallback cb, int32_t priority = 0);
	uint64_t subscribe(std::vector<uint64_t> topics, SubscribeCallback cb, int32_t priority = 0);
	
	void unregister(uint64_t topic, uint64_t id);
	void unregister(std::vector<uint64_t> topics, uint64_t id);

	void publish(uint64_t topic, ::google::protobuf::Message& msg);

private:
	uint64_t generateId();

	void subscribeImpl(uint64_t topic, uint64_t id, SubscribeCallback cb, int32_t priority);

public:
	uint64_t m_id = { 0 };
	ChannelMap m_topicMap;

	//std::mutex m_topicMutex;
};

typedef ThreadSafeSingleton<PubSub> PubSubSingleton;



} 
