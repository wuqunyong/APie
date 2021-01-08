#include "route_proxy.h"

#include "route_client.h"

namespace APie {

std::tuple<uint32_t, std::string> RouteProxy::init()
{
	// CMD
	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_LogicCmd, RouteProxy::onLogicCommnad);

	LogicCmdHandlerSingleton::get().init();
	LogicCmdHandlerSingleton::get().registerOnCmd("show_topo", "show_topology", RouteProxy::onShowTopology);

	// RPC
	APie::RPC::rpcInit();

	APie::Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OP_ROUTE_MSG_RESP_ADD_ROUTE, RouteProxy::handleRespAddRoute, ::route_register::MSG_RESP_ADD_ROUTE::default_instance());
	APie::Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OP_ROUTE_MSG_RESP_HEARTBEAT, RouteProxy::handleRespHeartbeat, ::route_register::ROUTE_MSG_RESP_HEARTBEAT::default_instance());

	APie::PubSubSingleton::get().subscribe(::pubsub::PT_DiscoveryNotice, RouteProxy::onDiscoveryNotice);

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> RouteProxy::start()
{
	APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> RouteProxy::ready()
{
	std::stringstream ss;
	ss << "Server Ready!";
	std::cout << ss.str() << std::endl;
	ASYNC_PIE_LOG("ServerStatus", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

void RouteProxy::exit()
{

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

std::shared_ptr<RouteClient> RouteProxy::findRouteClient(uint64_t iSerialNum)
{
	auto findIte = m_reverseMap.find(iSerialNum);
	if (findIte == m_reverseMap.end())
	{
		return nullptr;
	}

	return findRouteClient(findIte->second);
}

std::optional<EndPoint> RouteProxy::findEndPoint(uint64_t iSerialNum)
{
	auto findIte = m_reverseMap.find(iSerialNum);
	if (findIte == m_reverseMap.end())
	{
		return std::nullopt;
	}

	return std::make_optional(findIte->second);
}


bool RouteProxy::addRouteClient(const ::service_discovery::EndPointInstance& instance)
{
	auto ptrClient = std::make_shared<RouteClient>(instance);
	ptrClient->init();

	EndPoint point;
	point.type = instance.type();
	point.id = instance.id();

	m_connectedPool[point] = ptrClient;
	m_reverseMap[ptrClient->getSerialNum()] = point;
	
	return true;
}

bool RouteProxy::delRouteClient(EndPoint point)
{
	auto findIte = m_connectedPool.find(point);
	if (findIte != m_connectedPool.end())
	{
		findIte->second->close();

		auto iSerialNum = findIte->second->getSerialNum();
		m_connectedPool.erase(findIte);
		m_reverseMap.erase(iSerialNum);
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
	ss << "handleRespAddRoute|" << "iSerialNum:" << iSerialNum << ",response:" << response.ShortDebugString();

	if (response.status_code() == opcodes::StatusCode::SC_Ok)
	{
		std::shared_ptr<RouteClient> ptrRouteClient = RouteProxySingleton::get().findRouteClient(iSerialNum);
		if (ptrRouteClient)
		{
			ptrRouteClient->setState(APie::RouteClient::Registered);
			
			auto optPoint = RouteProxySingleton::get().findEndPoint(iSerialNum);
			if (optPoint)
			{
				EndPointMgrSingleton::get().addRoute(optPoint.value(), iSerialNum);
			}

			ss << ",state:" << ptrRouteClient->state();
		}

		ASYNC_PIE_LOG("RouteProxy/handleRespAddRoute", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());
	}
	else
	{
		ASYNC_PIE_LOG("RouteProxy/handleRespAddRoute", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());
	}
}

void RouteProxy::handleRespHeartbeat(uint64_t iSerialNum, const ::route_register::ROUTE_MSG_RESP_HEARTBEAT& response)
{
	std::stringstream ss;
	ss << "handleRespHeartbeat|" << "iSerialNum:" << iSerialNum << ",response:" << response.ShortDebugString();

	if (response.status_code() == opcodes::StatusCode::SC_Ok)
	{
		//ASYNC_PIE_LOG("RouteProxy/handleRespHeartbeat", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());
	}
	else
	{
		ASYNC_PIE_LOG("RouteProxy/handleRespHeartbeat", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());
	}
}

void RouteProxy::onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg)
{
	auto& command = dynamic_cast<::pubsub::LOGIC_CMD&>(msg);
	auto handlerOpt = LogicCmdHandlerSingleton::get().findCb(command.cmd());
	if (!handlerOpt.has_value())
	{
		return;
	}

	handlerOpt.value()(command);
}

void RouteProxy::onShowTopology(::pubsub::LOGIC_CMD& cmd)
{
	std::stringstream ss;
	for (const auto& items : RouteProxySingleton::get().connectedPool())
	{
		ss << "--> " << "type:" << items.first.type << "|id:" << items.first.id << "|status:" << (uint32_t)items.second->state() << "|isConnectted:" << items.second->isConnectted() << std::endl;
	}

	ASYNC_PIE_LOG("show_topology:%s", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());
}

void RouteProxy::onDiscoveryNotice(uint64_t topic, ::google::protobuf::Message& msg)
{
	std::stringstream ss;

	auto& refMsg = dynamic_cast<::pubsub::DISCOVERY_NOTICE&>(msg);
	ss << "onDiscoveryNotice|" << "topic:" << topic << ",refMsg:" << refMsg.ShortDebugString();
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

			if (point.type == ::common::EndPointType::EPT_Route_Proxy)
			{
				continue;
			}

			auto ptrClient = RouteProxySingleton::get().findRouteClient(point);
			if (ptrClient == nullptr)
			{
				RouteProxySingleton::get().addRouteClient(items);
			}
			else
			{
				bool bChange = false;
				if (items.ip() != ptrClient->getInstance().ip()
					|| items.port() != ptrClient->getInstance().port())
				{
					bChange = true;
				}

				if (bChange)
				{
					RouteProxySingleton::get().delRouteClient(point);
					RouteProxySingleton::get().addRouteClient(items);
				}
				//ptrClient->setInstance(items);
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

