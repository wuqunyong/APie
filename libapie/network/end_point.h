#pragma once

#include <stdint.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <optional>

#include "../network/i_poll_events.hpp"
#include "../network/client_proxy.h"

#include "../singleton/threadsafe_singleton.h"

#include "../pb_msg.h"




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

		//向ServiceRegistry注册服务
		void registerEndpoint();
		void unregisterEndpoint();

		void setState(State state);
		State state();

		void sendRegister(APie::ClientProxy* ptrClient, std::string registryAuth);
		void sendHeartbeat(APie::ClientProxy* ptrClient);

	public:
		static void handleRespRegisterInstance(uint64_t iSerialNum, const ::service_discovery::MSG_RESP_REGISTER_INSTANCE& response);
		static void handleNoticeInstance(uint64_t iSerialNum, const ::service_discovery::MSG_NOTICE_INSTANCE& notice);
		static void handleRespHeartbeat(uint64_t iSerialNum, const ::service_discovery::MSG_RESP_HEARTBEAT& response);
		

		static void handleAddRoute(uint64_t iSerialNum, const ::route_register::MSG_REQUEST_ADD_ROUTE& request);
		static void handleRouteHeartbeat(uint64_t iSerialNum, const ::route_register::MSG_REQUEST_HEARTBEAT& request);
		

		static void onClientPeerClose(uint64_t topic, ::google::protobuf::Message& msg);
		static void onServerPeerClose(uint64_t topic, ::google::protobuf::Message& msg);

		static std::shared_ptr<SelfRegistration> createSelfRegistration();

	private:

		State m_state = { Unregistered };
		//std::shared_ptr<ClientProxy> m_ptrClient = { nullptr };
	};


	struct EstablishedState
	{
		uint64_t iSerialNum = 0;
		uint32_t iState = 0;
		time_t iLastHeartbeat = 0;
	};

	class EndPointMgr
	{
	public:
		int registerEndpoint(::service_discovery::EndPointInstance instance);
		void unregisterEndpoint(EndPoint point);
		std::optional<::service_discovery::EndPointInstance> findEndpoint(EndPoint point);

		std::optional<::service_discovery::EndPointInstance> modulusEndpointById(uint32_t type, uint64_t matchId);

		std::map<EndPoint, ::service_discovery::EndPointInstance>& getEndpoints();
		std::vector<EndPoint> getEndpointsByType(uint32_t type);
		std::optional<uint64_t> getSerialNum(EndPoint point);

		void addRoute(const EndPoint& point, uint64_t iSerialNum);
		void delRoute(uint64_t iSerialNum);
		void updateRouteHeartbeat(uint64_t iSerialNum);

		std::vector<EndPoint> getEstablishedEndpointsByType(uint32_t type);
		std::optional<EndPoint> findRoute(uint64_t iSerialNum);

		void clear();

	private:
		//在注册中心已注册的节点
		std::map<EndPoint, ::service_discovery::EndPointInstance> m_endpoints;

		//已连接上的路由节点
		std::map<EndPoint, EstablishedState> m_establishedPoints;
		std::map<uint64_t, EndPoint> m_reversePoints; 
	};

	using EndPointMgrSingleton = ThreadSafeSingleton<EndPointMgr>;
}    

