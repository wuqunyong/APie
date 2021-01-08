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
		std::tuple<uint32_t, std::string> init();
		std::tuple<uint32_t, std::string> start();
		std::tuple<uint32_t, std::string> ready();
		void exit();

	public:
		static void handleRequestRegisterInstance(uint64_t iSerialNum, const ::service_discovery::MSG_REQUEST_REGISTER_INSTANCE& request);
		static void handleRequestHeartbeat(uint64_t iSerialNum, const ::service_discovery::MSG_REQUEST_HEARTBEAT& request);
		

		static void onServerPeerClose(uint64_t topic, ::google::protobuf::Message& msg);

	public:
		void update();

		bool updateInstance(uint64_t iSerialNum, const ::service_discovery::EndPointInstance& instance);
		bool updateHeartbeat(uint64_t iSerialNum);
		bool deleteBySerialNum(uint64_t iSerialNum);

		void checkTimeout();
		void broadcast();

		void addUpdateTimer(uint64_t interval);
		void disableUpdateTimer();

	public:
		std::map<uint64_t, RegisteredEndPoint> m_registered;
		std::map<EndPoint, uint64_t> m_pointMap;
		uint32_t m_serviceTimeout = 300;

		Event::TimerPtr m_updateTimer;
	};


	//typedef APie::ThreadSafeSingleton<ServiceRegistry> ServiceRegistrySingleton;

	using ServiceRegistrySingleton = ThreadSafeSingleton<ServiceRegistry>;
}
