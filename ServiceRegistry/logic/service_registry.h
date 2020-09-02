#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

#include "apie.h"

#include "../../PBMsg/service_discovery.pb.h"


namespace APie {

	struct RegisteredEndPoint
	{
		uint64_t addTime = {0};
		uint64_t modifyTime = {0};
		::service_discovery::EndPointInstance instance;
	};

	class ServiceRegistry
	{
	public:
		void init();

	public:
		static void handleRequestAddInstance(uint64_t iSerialNum, const ::service_discovery::MSG_REQUEST_ADD_INSTANCE& response);

		static void onServerPeerClose(uint64_t topic, ::google::protobuf::Message& msg);

	public:
		bool updateInstance(uint64_t iSerialNum, const ::service_discovery::EndPointInstance& instance);
		bool deleteBySerialNum(uint64_t iSerialNum);

		void broadcast();

	public:
		std::map<uint64_t, RegisteredEndPoint> m_registered;
		std::map<EndPoint, uint64_t> m_pointMap;
	};


	//typedef APie::ThreadSafeSingleton<ServiceRegistry> ServiceRegistrySingleton;

	using ServiceRegistrySingleton = ThreadSafeSingleton<ServiceRegistry>;
}
