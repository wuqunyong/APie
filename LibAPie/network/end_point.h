#pragma once

#include <stdint.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <optional>

#include "../network/i_poll_events.hpp"
#include "../network/client_proxy.h"

#include "../singleton/threadsafe_singleton.h"

#include "../../../PBMsg/service_discovery.pb.h"




namespace APie
{
	class SelfRegistration : public std::enable_shared_from_this<SelfRegistration>
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

		void setState(State state);

	public:
		static void handleRespAddInstance(uint64_t iSerialNum, const ::service_discovery::MSG_RESP_ADD_INSTANCE& response);
		static void handleNoticeInstance(uint64_t iSerialNum, const ::service_discovery::MSG_NOTICE_INSTANCE& notice);
		
	private:
		State m_state = { Unregistered };
		std::shared_ptr<ClientProxy> m_ptrClient = { nullptr };
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

