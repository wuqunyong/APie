#include "end_point.h"

#include "ctx.h"
#include "client_proxy.h"

#include "../pb_msg.h"

#include "../api/pb_handler.h"
#include "../api/pubsub.h"

#include "../rpc/init.h"

#include "output_stream.h"
#include <sstream>

namespace APie{

void SelfRegistration::init()
{
	//ServiceRegistry
	APie::Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OP_DISCOVERY_MSG_RESP_REGISTER_INSTANCE, SelfRegistration::handleRespRegisterInstance);
	APie::Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OP_DISCOVERY_MSG_NOTICE_INSTANCE, SelfRegistration::handleNoticeInstance);
	APie::Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OP_DISCOVERY_MSG_RESP_HEARTBEAT, SelfRegistration::handleRespHeartbeat);


	//RouteProxy
	APie::Api::OpcodeHandlerSingleton::get().server.bind(::opcodes::OP_ROUTE_MSG_REQUEST_ADD_ROUTE, SelfRegistration::handleAddRoute);
	APie::Api::OpcodeHandlerSingleton::get().server.bind(::opcodes::OP_ROUTE_MSG_REQUEST_HEARTBEAT, SelfRegistration::handleRouteHeartbeat);

	//PubSub
	APie::PubSubSingleton::get().subscribe(::pubsub::PT_ClientPeerClose, SelfRegistration::onClientPeerClose);
	APie::PubSubSingleton::get().subscribe(::pubsub::PT_ServerPeerClose, SelfRegistration::onServerPeerClose);

	this->registerEndpoint();
}

void SelfRegistration::registerEndpoint()
{
	auto identityType = APie::CtxSingleton::get().getServerType();

	std::set<uint32_t> needRegister;

#ifdef USE_NATS_PROXY
#else
	needRegister.insert(::common::EndPointType::EPT_Route_Proxy);
	needRegister.insert(::common::EndPointType::EPT_Login_Server);
	needRegister.insert(::common::EndPointType::EPT_Gateway_Server);
	needRegister.insert(::common::EndPointType::EPT_Scene_Server);
	needRegister.insert(::common::EndPointType::EPT_DB_ACCOUNT_Proxy);
	needRegister.insert(::common::EndPointType::EPT_DB_ROLE_Proxy);
#endif

	if (needRegister.count(identityType) == 0)
	{
		return;
	}

	if (APie::CtxSingleton::get().getConfigs()->service_registry.address.empty())
	{
		std::stringstream ss;
		ss << "service_registry empty";

		PIE_LOG("SelfRegistration/registerEndpoint", PIE_CYCLE_DAY, PIE_WARNING, ss.str().c_str());
		PANIC_ABORT(ss.str().c_str());
	}

	std::string ip = APie::CtxSingleton::get().getConfigs()->service_registry.address;
	uint16_t port = APie::CtxSingleton::get().getConfigs()->service_registry.port_value;
	std::string registryAuth = APie::CtxSingleton::get().getConfigs()->service_registry.auth;
	uint16_t type = APie::CtxSingleton::get().getConfigs()->service_registry.type;
	
	uint32_t maskFlag = 0;

	auto ptrSelf = this->shared_from_this();
	auto ptrClient = APie::ClientProxy::createClientProxy();
	auto connectCb = [ptrSelf, registryAuth](APie::ClientProxy* ptrClient, uint32_t iResult) {
		if (iResult == 0)
		{
			ptrSelf->sendRegister(ptrClient, registryAuth);
			ptrSelf->setState(APie::SelfRegistration::Registering);
		}
		return true;
	};
	ptrClient->connect(ip, port, static_cast<APie::ProtocolType>(type), maskFlag, connectCb);

	auto heartbeatCb = [ptrSelf, registryAuth](APie::ClientProxy *ptrClient) {
		ptrClient->addHeartbeatTimer(3000);

		if (ptrSelf->state() != APie::SelfRegistration::Registered)
		{
			ptrSelf->sendRegister(ptrClient, registryAuth);
		}
		else
		{
			ptrSelf->sendHeartbeat(ptrClient);
		}
	};
	ptrClient->setHeartbeatCb(heartbeatCb);
	ptrClient->addHeartbeatTimer(1000);
	ptrClient->addReconnectTimer(1000);
	ptrClient.reset();
}

void SelfRegistration::unregisterEndpoint()
{

}

void SelfRegistration::sendRegister(APie::ClientProxy* ptrClient, std::string registryAuth)
{
	uint32_t type = APie::CtxSingleton::get().getServerType();
	uint32_t id = APie::CtxSingleton::get().getServerId();
	std::string auth = APie::CtxSingleton::get().getConfigs()->identify.auth;
	std::string ip = APie::CtxSingleton::get().getConfigs()->identify.ip;
	uint32_t port = APie::CtxSingleton::get().getConfigs()->identify.port;
	uint32_t codec_type = APie::CtxSingleton::get().getConfigs()->identify.codec_type;

	::service_discovery::MSG_REQUEST_REGISTER_INSTANCE request;
	request.mutable_instance()->set_type(static_cast<::common::EndPointType>(type));
	request.mutable_instance()->set_id(id);
	request.mutable_instance()->set_auth(auth);
	request.mutable_instance()->set_ip(ip);
	request.mutable_instance()->set_port(port);
	request.mutable_instance()->set_codec_type(codec_type);
	request.mutable_instance()->set_mask_flag(0); // 节点间不需要压缩，加密
	//request.mutable_instance()->set_db_id(db_id);
	request.set_auth(registryAuth);

	ptrClient->sendMsg(::opcodes::OP_DISCOVERY_MSG_REQUEST_REGISTER_INSTANCE, request);
}

void SelfRegistration::sendHeartbeat(APie::ClientProxy* ptrClient)
{
	::service_discovery::MSG_REQUEST_HEARTBEAT request;

	ptrClient->sendMsg(::opcodes::OP_DISCOVERY_MSG_REQUEST_HEARTBEAT, request);
}


void SelfRegistration::setState(State state)
{
	m_state = state;
}

SelfRegistration::State SelfRegistration::state()
{
	return m_state;
}


int EndPointMgr::registerEndpoint(::service_discovery::EndPointInstance instance)
{
	EndPoint point;
	point.type = instance.type();
	point.id = instance.id();

	m_endpoints[point] = instance;

	return 0;
}
void EndPointMgr::unregisterEndpoint(EndPoint point)
{
	auto findIte = m_endpoints.find(point);
	if (findIte != m_endpoints.end())
	{
		m_endpoints.erase(findIte);
	}
}
std::optional<::service_discovery::EndPointInstance> EndPointMgr::findEndpoint(EndPoint point)
{
	auto findIte = m_endpoints.find(point);
	if (findIte != m_endpoints.end())
	{
		return std::make_optional(findIte->second);
	}

	return std::nullopt;
}

std::optional<::service_discovery::EndPointInstance> EndPointMgr::modulusEndpointById(uint32_t type, uint64_t matchId)
{
	std::vector<EndPoint> pointList = this->getEndpointsByType(type);
	if (pointList.empty())
	{
		return std::nullopt;
	}

	uint32_t iSize = pointList.size();
	uint32_t iIndex = matchId % iSize;
	return findEndpoint(pointList[iIndex]);
}

std::map<EndPoint, ::service_discovery::EndPointInstance>& EndPointMgr::getEndpoints()
{
	return m_endpoints;
}

std::vector<EndPoint> EndPointMgr::getEndpointsByType(uint32_t type)
{
	std::vector<EndPoint> result;
	for (const auto& items : m_endpoints)
	{
		if (items.first.type == type)
		{
			result.push_back(items.first);
		}
	}

	return result;
}

std::vector<EndPoint> EndPointMgr::getEstablishedEndpointsByType(uint32_t type)
{
	std::vector<EndPoint> result;
	for (const auto& items : m_establishedPoints)
	{
		auto pointIte = m_endpoints.find(items.first);
		if (pointIte == m_endpoints.end())
		{
			continue;
		}

		if (items.first.type == type)
		{
			result.push_back(items.first);
		}
	}

	return result;
}

std::optional<uint64_t> EndPointMgr::getSerialNum(EndPoint point)
{
	auto pointIte = m_endpoints.find(point);
	if (pointIte == m_endpoints.end())
	{
		return std::nullopt;
	}

	auto findIte = m_establishedPoints.find(point);
	if (findIte != m_establishedPoints.end())
	{
		return std::make_optional(findIte->second.iSerialNum);
	}

	return std::nullopt;
}

void EndPointMgr::addRoute(const EndPoint& point, uint64_t iSerialNum)
{
	EstablishedState state;
	state.iSerialNum = iSerialNum;
	state.iLastHeartbeat = time(NULL);

	m_establishedPoints[point] = state;
	m_reversePoints[iSerialNum] = point;
}

void EndPointMgr::delRoute(uint64_t iSerialNum)
{
	auto findIte = m_reversePoints.find(iSerialNum);
	if (findIte == m_reversePoints.end())
	{
		return;
	}

	m_establishedPoints.erase(findIte->second);
	m_reversePoints.erase(findIte);
}

void EndPointMgr::updateRouteHeartbeat(uint64_t iSerialNum)
{
	auto findIte = m_reversePoints.find(iSerialNum);
	if (findIte == m_reversePoints.end())
	{
		return;
	}

	auto updateIte = m_establishedPoints.find(findIte->second);
	if (updateIte == m_establishedPoints.end())
	{
		return;
	}

	auto iCurTime = time(NULL);
	updateIte->second.iLastHeartbeat = iCurTime;
}

std::optional<EndPoint> EndPointMgr::findRoute(uint64_t iSerialNum)
{
	auto findIte = m_reversePoints.find(iSerialNum);
	if (findIte == m_reversePoints.end())
	{
		return std::nullopt;
	}

	return std::make_optional(findIte->second);
}

void EndPointMgr::clear()
{
	this->m_endpoints.clear();
	//this->m_establishedPoints.clear();
}


void SelfRegistration::handleRespRegisterInstance(uint64_t iSerialNum, const ::service_discovery::MSG_RESP_REGISTER_INSTANCE& response)
{
	std::stringstream ss;
	ss << "iSerialNum:" << iSerialNum << ",response:" << response.ShortDebugString();

	if (response.status_code() != opcodes::StatusCode::SC_Ok)
	{
		ASYNC_PIE_LOG("SelfRegistration/handleRespRegisterInstance", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());
		return;
	}
	else
	{
		ASYNC_PIE_LOG("SelfRegistration/handleRespRegisterInstance", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());
		APie::CtxSingleton::get().getEndpoint()->setState(APie::SelfRegistration::Registered);
	}
}

void SelfRegistration::handleNoticeInstance(uint64_t iSerialNum, const ::service_discovery::MSG_NOTICE_INSTANCE& notice)
{
	std::stringstream ss;
	ss << "iSerialNum:" << iSerialNum << ",notice:" << notice.ShortDebugString();
	ASYNC_PIE_LOG("SelfRegistration/handleNoticeInstance", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	switch (notice.mode())
	{
	case service_discovery::UM_Full:
	{
		if (notice.status() == service_discovery::RS_Forwarding)
		{
			EndPointMgrSingleton::get().clear();
		}

		for (const auto& items : notice.add_instance())
		{
			EndPointMgrSingleton::get().registerEndpoint(items);
		}
		break;
	}
	case service_discovery::UM_Incremental:
	{
		break;
	}
	default:
		break;
	}

	::pubsub::DISCOVERY_NOTICE msg;
	*msg.mutable_notice() = notice;
	PubSubSingleton::get().publish(::pubsub::PUB_TOPIC::PT_DiscoveryNotice, msg);
}

void SelfRegistration::handleRespHeartbeat(uint64_t iSerialNum, const ::service_discovery::MSG_RESP_HEARTBEAT& response)
{
	std::stringstream ss;
	ss << "iSerialNum:" << iSerialNum << ",response:" << response.ShortDebugString();
	if (response.status_code() == opcodes::StatusCode::SC_Ok)
	{
		//ASYNC_PIE_LOG("SelfRegistration/handleRespHeartbeat", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());
	}
	else
	{
		ASYNC_PIE_LOG("SelfRegistration/handleRespHeartbeat", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());
	}
}

void SelfRegistration::handleAddRoute(uint64_t iSerialNum, const ::route_register::MSG_REQUEST_ADD_ROUTE& request)
{
	EndPoint point;
	point.type = request.instance().type();
	point.id = request.instance().id();

	auto identify = ::APie::CtxSingleton::get().identify();

	::route_register::MSG_RESP_ADD_ROUTE response;
	response.mutable_target()->set_type(static_cast<::common::EndPointType>(identify.type));
	response.mutable_target()->set_id(identify.id);
	response.mutable_target()->set_auth(identify.auth);
	*response.mutable_route() = request.instance();

	auto findOpt = EndPointMgrSingleton::get().findEndpoint(point);
	if (!findOpt.has_value())
	{
		response.set_status_code(opcodes::SC_Route_InvalidPoint);
		APie::Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_ROUTE_MSG_RESP_ADD_ROUTE, response);
		return;
	}

	auto auth = findOpt.value().auth();
	if (!auth.empty() && auth != request.instance().auth())
	{
		response.set_status_code(opcodes::SC_Route_AuthError);
		APie::Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_ROUTE_MSG_RESP_ADD_ROUTE, response);
		return;
	}

	response.set_status_code(opcodes::SC_Ok);
	APie::Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_ROUTE_MSG_RESP_ADD_ROUTE, response);

	EndPointMgrSingleton::get().addRoute(point, iSerialNum);
}

void SelfRegistration::handleRouteHeartbeat(uint64_t iSerialNum, const ::route_register::MSG_REQUEST_HEARTBEAT& request)
{
	auto optPoint = EndPointMgrSingleton::get().findRoute(iSerialNum);

	::route_register::MSG_RESP_HEARTBEAT response;
	if (optPoint)
	{
		response.set_status_code(opcodes::SC_Ok);
		EndPointMgrSingleton::get().updateRouteHeartbeat(iSerialNum);
	}
	else
	{
		response.set_status_code(opcodes::SC_Route_Unregistered);
	}
	APie::Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_ROUTE_MSG_RESP_HEARTBEAT, response);
}

void SelfRegistration::onClientPeerClose(uint64_t topic, ::google::protobuf::Message& msg)
{
	std::stringstream ss;
	auto& refMsg = dynamic_cast<::pubsub::CLIENT_PEER_CLOSE&>(msg);
	ss << "topic:" << topic << ",refMsg:" << refMsg.ShortDebugString();
	ASYNC_PIE_LOG("SelfRegistration/onClientPeerClose", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	uint64_t iSerialNum = refMsg.serial_num();
	auto clientProxy = APie::ClientProxy::findClientProxy(iSerialNum);
	if (clientProxy)
	{
		clientProxy->setHadEstablished(ClientProxy::CONNECT_CLOSE);
		//clientProxy->onConnect(refMsg.result());

		if (!clientProxy->reconnectTimer()->enabled())
		{
			clientProxy->addReconnectTimer(3000);
		}
	}
}

void SelfRegistration::onServerPeerClose(uint64_t topic, ::google::protobuf::Message& msg)
{
	std::stringstream ss;

	auto& refMsg = dynamic_cast<::pubsub::SERVER_PEER_CLOSE&>(msg);
	ss << "topic:" << topic << ",refMsg:" << refMsg.ShortDebugString();
	ASYNC_PIE_LOG("SelfRegistration/onServerPeerClose", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	uint64_t iSerialNum = refMsg.serial_num();
	EndPointMgrSingleton::get().delRoute(iSerialNum);
}

std::shared_ptr<SelfRegistration> SelfRegistration::createSelfRegistration()
{
	return std::make_shared<SelfRegistration>();
}

}
