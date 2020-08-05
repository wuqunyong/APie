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

	class RouteClient : public std::enable_shared_from_this<RouteClient>
	{		
	public:
		RouteClient(::service_discovery::EndPointInstance instance);

		enum class State
		{
			Unregistered = 0,
			Registering,
			registered,
		};

		void init();
		void close();
		void setInstance(const ::service_discovery::EndPointInstance& instance);
		::service_discovery::EndPointInstance& getInstance();
		std::shared_ptr<ClientProxy> clientProxy();

	private:
		EndPoint m_point;
		::service_discovery::EndPointInstance m_instance;
		std::shared_ptr<ClientProxy> m_clientProxy;
	};

	class RouteProxy
	{
	public:
		void init();

		std::shared_ptr<RouteClient> findRouteClient(EndPoint point);
		bool addRouteClient(const ::service_discovery::EndPointInstance& instance);
		bool delRouteClient(EndPoint point);

		std::map<EndPoint, std::shared_ptr<RouteClient>>& connectedPool();

	public:
		static void handleRespAddRoute(uint64_t iSerialNum, const ::route_register::MSG_RESP_ADD_ROUTE& response);

		static void onDiscoveryNotice(uint64_t topic, ::google::protobuf::Message& msg);

	private:
		std::map<EndPoint, std::shared_ptr<RouteClient>> m_connectedPool;
	};


	using RouteProxySingleton = ThreadSafeSingleton<RouteProxy>;
}
