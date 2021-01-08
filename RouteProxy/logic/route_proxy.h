#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>
#include <atomic>
#include <optional>

#include "apie.h"
#include "../../PBMsg/service_discovery.pb.h"



namespace APie {

	class RouteClient;

	class RouteProxy
	{
	public:
		std::tuple<uint32_t, std::string> init();
		std::tuple<uint32_t, std::string> start();
		std::tuple<uint32_t, std::string> ready();
		void exit();

		std::shared_ptr<RouteClient> findRouteClient(EndPoint point);
		std::shared_ptr<RouteClient> findRouteClient(uint64_t iSerialNum);
		std::optional<EndPoint> findEndPoint(uint64_t iSerialNum);

		bool addRouteClient(const ::service_discovery::EndPointInstance& instance);
		bool delRouteClient(EndPoint point);

		std::map<EndPoint, std::shared_ptr<RouteClient>>& connectedPool();
		

	public:
		// CMD
		static void onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg);

		static void onShowTopology(::pubsub::LOGIC_CMD& cmd);

		// PubSub
		static void onDiscoveryNotice(uint64_t topic, ::google::protobuf::Message& msg);


		static void handleRespAddRoute(uint64_t iSerialNum, const ::route_register::MSG_RESP_ADD_ROUTE& response);
		static void handleRespHeartbeat(uint64_t iSerialNum, const ::route_register::MSG_RESP_HEARTBEAT& response);
		

	private:
		std::map<EndPoint, std::shared_ptr<RouteClient>> m_connectedPool;
		std::map<uint64_t, EndPoint> m_reverseMap;
	};


	using RouteProxySingleton = ThreadSafeSingleton<RouteProxy>;
}
