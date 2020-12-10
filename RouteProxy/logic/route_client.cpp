#include "route_client.h"

namespace APie {

std::atomic<uint32_t> RouteClient::s_id(0);

RouteClient::RouteClient(::service_discovery::EndPointInstance instance)
{
	m_point.type = instance.type();
	m_point.id = instance.id();

	m_instance = instance;

	s_id++;
	m_id = s_id;

	std::stringstream ss;
	ss << "construcotr|m_id:" << m_id;
	ASYNC_PIE_LOG("RouteClient", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());
}

RouteClient::~RouteClient()
{
	std::stringstream ss;
	ss << "destructor|m_id:" << m_id;
	ASYNC_PIE_LOG("RouteClient", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());
}

void RouteClient::init()
{
	auto ptrSelf = this->shared_from_this();

	std::weak_ptr<RouteClient> weakPtr(ptrSelf);

	auto ip = m_instance.ip();
	auto port = m_instance.port();
	auto type = m_instance.codec_type();
	auto maskFlag = m_instance.mask_flag();

	m_clientProxy = APie::ClientProxy::createClientProxy();
	auto connectCb = [weakPtr](APie::ClientProxy* ptrClient, uint32_t iResult) {
		auto sharedPtr = weakPtr.lock();
		if (!sharedPtr)
		{
			return false;
		}

		if (iResult == 0)
		{
			sharedPtr->sendAddRoute(ptrClient);
			sharedPtr->setState(RouteClient::Registering);
			return true;
		}

		auto instance = sharedPtr->getInstance();
		auto ip = instance.ip();
		auto port = instance.port();
		auto type = instance.codec_type();
		ptrClient->resetConnect(ip, port, static_cast<APie::ProtocolType>(type));

		return true;
	};
	m_clientProxy->connect(ip, port, static_cast<APie::ProtocolType>(type), maskFlag, connectCb);

	auto heartbeatCb = [weakPtr](APie::ClientProxy *ptrClient) {
		ptrClient->addHeartbeatTimer(3000);

		auto sharedPtr = weakPtr.lock();
		if (!sharedPtr)
		{
			return;
		}

		if (sharedPtr->state() != APie::RouteClient::Registered)
		{
			sharedPtr->sendAddRoute(ptrClient);
		}
		else
		{
			sharedPtr->sendHeartbeat(ptrClient);
		}
	};
	m_clientProxy->setHeartbeatCb(heartbeatCb);
	m_clientProxy->addHeartbeatTimer(1000);
	m_clientProxy->addReconnectTimer(1000);


	std::stringstream ss;
	ss << "init|m_id:" << m_id << "|iSerialNum:" << m_clientProxy->getSerialNum() << "|use_count:" << ptrSelf.use_count();
	ASYNC_PIE_LOG("RouteClient", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());
}

void RouteClient::close()
{
	if (m_clientProxy != nullptr)
	{
		m_clientProxy->onActiveClose();
	}
}

void RouteClient::setInstance(const ::service_discovery::EndPointInstance& instance)
{
	m_instance = instance;
}

::service_discovery::EndPointInstance& RouteClient::getInstance()
{
	return m_instance;
}

std::shared_ptr<ClientProxy> RouteClient::clientProxy()
{
	return m_clientProxy;
}

uint64_t RouteClient::getSerialNum()
{
	uint64_t iSerialNum = 0;
	if (m_clientProxy)
	{
		iSerialNum = m_clientProxy->getSerialNum();
	}

	return iSerialNum;
}

bool RouteClient::isConnectted()
{
	if (m_clientProxy)
	{
		return m_clientProxy->isConnectted();
	}

	return false;
}

RouteClient::State RouteClient::state()
{
	return m_state;
}

void RouteClient::setState(State value)
{
	m_state = value;
}

void RouteClient::sendAddRoute(APie::ClientProxy* ptrClient)
{
	uint32_t type = APie::CtxSingleton::get().identify().type;
	uint32_t id = APie::CtxSingleton::get().identify().id;
	std::string auth = APie::CtxSingleton::get().identify().auth;

	::route_register::MSG_REQUEST_ADD_ROUTE request;
	auto ptrAdd = request.mutable_instance();
	ptrAdd->set_type(static_cast<::common::EndPointType>(type));
	ptrAdd->set_id(id);
	ptrAdd->set_auth(auth);
	ptrClient->sendMsg(::opcodes::OP_MSG_REQUEST_ADD_ROUTE, request);
}

void RouteClient::sendHeartbeat(APie::ClientProxy* ptrClient)
{
	::route_register::ROUTE_MSG_REQUEST_HEARTBEAT request;

	ptrClient->sendMsg(::opcodes::OP_ROUTE_MSG_REQUEST_HEARTBEAT, request);
}

}

