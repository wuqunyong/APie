#pragma once

#include <stdint.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <optional>

#include "../network/i_poll_events.hpp"
#include "../singleton/threadsafe_singleton.h"

#include "../../../PBMsg/service_discovery.pb.h"



namespace APie
{
	class SelfRegistration
	{
	public:
		enum State
		{
			Unregistered = 0,
			Registering,
			Registered
		};

		void init();

		void registerEndpoint();
		void unregisterEndpoint();
		void heartbeat();

	private:
		State m_state = { Unregistered };
	};


	class EndPointMgr
	{
	public:
		int registerEndpoint(::service_discovery::EndPointInstance instance);
		void unregisterEndpoint(EndPoint point);
		std::optional<::service_discovery::EndPointInstance> findEndpoint(EndPoint point);

	private:
		std::map<EndPoint, ::service_discovery::EndPointInstance> m_endpoints;
	};


	typedef ThreadSafeSingleton<EndPointMgr> EndPointMgrSingleton;
}    

