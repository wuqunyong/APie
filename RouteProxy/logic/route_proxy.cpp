#include "route_proxy.h"

namespace APie {

RouteClient::RouteClient(::service_discovery::EndPointInstance instance)
{
	m_point.type = instance.type();
	m_point.id = instance.id();

	m_instance = instance;
}

void RouteClient::init()
{
	auto ptrSelf = this->shared_from_this();

	auto ip = m_instance.ip();
	auto port = m_instance.port();
	auto type = m_instance.codec_type();

	m_clientProxy = APie::ClientProxy::createClientProxy();
	auto connectCb = [ptrSelf](std::shared_ptr<APie::ClientProxy> self, uint32_t iResult) {
		if (iResult == 0)
		{
			uint32_t type = APie::CtxSingleton::get().identify().type;
			uint32_t id = APie::CtxSingleton::get().identify().id;
			std::string auth = APie::CtxSingleton::get().identify().auth;

			::route_register::MSG_REQUEST_ADD_ROUTE request;
			auto ptrAdd = request.mutable_instance();
			ptrAdd->set_type(static_cast<::service_discovery::EndPointType>(type));
			ptrAdd->set_id(id);
			ptrAdd->set_auth(auth);

			self->sendMsg(::opcodes::OP_MSG_REQUEST_ADD_ROUTE, request);

		}
		return true;
	};
	m_clientProxy->connect(ip, port, static_cast<APie::ProtocolType>(type), connectCb);

	auto heartbeatCb = [](APie::ClientProxy *ptrClient) {
		ptrClient->addHeartbeatTimer(3000);
	};
	m_clientProxy->setHeartbeatCb(heartbeatCb);
	m_clientProxy->addHeartbeatTimer(1000);
	m_clientProxy->addReconnectTimer(1000);
}

void RouteProxy::init()
{
	APie::Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OP_MSG_RESP_ADD_ROUTE, RouteProxy::handleRespAddRoute, ::route_register::MSG_RESP_ADD_ROUTE::default_instance());
}

std::shared_ptr<RouteClient> RouteProxy::findRouteClient(EndPoint point)
{
	auto findIte = m_connectedPool.find(point);
	if (findIte == m_connectedPool.end())
	{
		return nullptr;
	}

	return findIte->second;
}

void RouteProxy::handleRespAddRoute(uint64_t iSerialNum, const ::route_register::MSG_RESP_ADD_ROUTE& response)
{

}

}

