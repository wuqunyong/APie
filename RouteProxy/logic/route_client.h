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

		bool isConnectted();

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
}
