#include "route_proxy.h"

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

	m_clientProxy = APie::ClientProxy::createClientProxy();
	auto connectCb = [weakPtr](APie::ClientProxy* ptrClient, uint32_t iResult) {
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
			ptrClient->sendMsg(::opcodes::OP_MSG_REQUEST_ADD_ROUTE, request);

			return true;
		}

		auto sharedPtr = weakPtr.lock();
		if (!sharedPtr)
		{
			return false;
		}

		auto instance = sharedPtr->getInstance();
		auto ip = instance.ip();
		auto port = instance.port();
		auto type = instance.codec_type();
		ptrClient->resetConnect(ip, port, static_cast<APie::ProtocolType>(type));

		return true;
	};
	m_clientProxy->connect(ip, port, static_cast<APie::ProtocolType>(type), connectCb);

	auto heartbeatCb = [](APie::ClientProxy *ptrClient) {
		ptrClient->addHeartbeatTimer(3000);
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

void RouteProxy::init()
{
	APie::Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OP_MSG_RESP_ADD_ROUTE, RouteProxy::handleRespAddRoute, ::route_register::MSG_RESP_ADD_ROUTE::default_instance());

	APie::PubSubSingleton::get().subscribe(::pubsub::PT_DiscoveryNotice, RouteProxy::onDiscoveryNotice);
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

bool RouteProxy::addRouteClient(const ::service_discovery::EndPointInstance& instance)
{
	auto ptrClient = std::make_shared<RouteClient>(instance);
	ptrClient->init();

	EndPoint point;
	point.type = instance.type();
	point.id = instance.id();

	m_connectedPool[point] = ptrClient;
	
	return true;
}

bool RouteProxy::delRouteClient(EndPoint point)
{
	auto findIte = m_connectedPool.find(point);
	if (findIte != m_connectedPool.end())
	{
		findIte->second->close();
		m_connectedPool.erase(findIte);
	}

	return true;
}

std::map<EndPoint, std::shared_ptr<RouteClient>>& RouteProxy::connectedPool()
{
	return m_connectedPool;
}

void RouteProxy::handleRespAddRoute(uint64_t iSerialNum, const ::route_register::MSG_RESP_ADD_ROUTE& response)
{
	std::stringstream ss;
	ss << "handleRespAddRoute|" << "iSerialNum:" << iSerialNum << ",response:" << response.DebugString();
	ASYNC_PIE_LOG("RouteProxy/handleRespAddRoute", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());
}

void RouteProxy::onDiscoveryNotice(uint64_t topic, ::google::protobuf::Message& msg)
{
	std::stringstream ss;

	auto& refMsg = dynamic_cast<::pubsub::DISCOVERY_NOTICE&>(msg);
	ss << "onDiscoveryNotice|" << "topic:" << topic << ",refMsg:" << refMsg.DebugString();
	ASYNC_PIE_LOG("RouteProxy/onDiscoveryNotice", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	switch (refMsg.notice().mode())
	{
	case ::service_discovery::UM_Full:
	{
		std::set<EndPoint> allInstance;

		for (const auto& items : refMsg.notice().add_instance())
		{
			EndPoint point;
			point.type = items.type();
			point.id = items.id();

			allInstance.insert(point);

			auto ptrClient = RouteProxySingleton::get().findRouteClient(point);
			if (ptrClient == nullptr)
			{
				RouteProxySingleton::get().addRouteClient(items);
			}
			else
			{
				ptrClient->setInstance(items);
			}
		}

		std::vector<EndPoint> delInstance;

		for (const auto& items : RouteProxySingleton::get().connectedPool())
		{
			if (allInstance.count(items.first) == 0)
			{
				delInstance.push_back(items.first);
			}
		}

		for (const auto& items : delInstance)
		{
			RouteProxySingleton::get().delRouteClient(items);
		}

		break;
	}
	default:
		break;
	}
}

}

