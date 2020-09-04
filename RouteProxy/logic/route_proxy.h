#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>
#include <atomic>

#include "apie.h"
#include "../../PBMsg/service_discovery.pb.h"


namespace APie {

	class RouteClient : public std::enable_shared_from_this<RouteClient>
	{		
	public:
		RouteClient(::service_discovery::EndPointInstance instance);
		~RouteClient();

		enum State
		{
			Unregistered = 0,
			Registering,
			Registered,
		};

		void init();
		void close();
		void setInstance(const ::service_discovery::EndPointInstance& instance);
		::service_discovery::EndPointInstance& getInstance();
		std::shared_ptr<ClientProxy> clientProxy();

		uint64_t getSerialNum();

		State state();
		void setState(State value);

		void sendAddRoute(APie::ClientProxy* ptrClient);
		void sendHeartbeat(APie::ClientProxy* ptrClient);

	private:
		uint32_t m_id;
		EndPoint m_point;
		::service_discovery::EndPointInstance m_instance;
		std::shared_ptr<ClientProxy> m_clientProxy;
		State m_state = { Unregistered };

		static std::atomic<uint32_t> s_id;
	};

	class RouteProxy
	{
	public:
		void init();
		void start();
		void exit();

		std::shared_ptr<RouteClient> findRouteClient(EndPoint point);
		std::shared_ptr<RouteClient> findRouteClient(uint64_t iSerialNum);

		bool addRouteClient(const ::service_discovery::EndPointInstance& instance);
		bool delRouteClient(EndPoint point);

		std::map<EndPoint, std::shared_ptr<RouteClient>>& connectedPool();

	public:
		static void handleRespAddRoute(uint64_t iSerialNum, const ::route_register::MSG_RESP_ADD_ROUTE& response);
		static void handleRespHeartbeat(uint64_t iSerialNum, const ::route_register::ROUTE_MSG_RESP_HEARTBEAT& response);
		

		static void onDiscoveryNotice(uint64_t topic, ::google::protobuf::Message& msg);

	private:
		std::map<EndPoint, std::shared_ptr<RouteClient>> m_connectedPool;
		std::map<uint64_t, EndPoint> m_reverseMap;
	};


	using RouteProxySingleton = ThreadSafeSingleton<RouteProxy>;
}
