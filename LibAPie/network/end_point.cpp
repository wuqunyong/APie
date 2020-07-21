#include "end_point.h"

#include "Ctx.h"
#include "client_proxy.h"

#include "../../../PBMsg/opcodes.pb.h"
#include "../../../PBMsg/pubsub.pb.h"

#include "../api/pb_handler.h"
#include "../api/pubsub.h"

#include "output_stream.h"


namespace APie{

void SelfRegistration::init()
{
	APie::Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OP_MSG_RESP_ADD_INSTANCE, SelfRegistration::handleRespAddInstance, ::service_discovery::MSG_RESP_ADD_INSTANCE::default_instance());
	APie::Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OP_MSG_NOTICE_INSTANCE, SelfRegistration::handleNoticeInstance, ::service_discovery::MSG_NOTICE_INSTANCE::default_instance());

	APie::Api::OpcodeHandlerSingleton::get().server.bind(::opcodes::OP_MSG_REQUEST_ADD_ROUTE, SelfRegistration::handleAddRoute, ::route_register::MSG_REQUEST_ADD_ROUTE::default_instance());


	APie::PubSubSingleton::get().subscribe(::pubsub::PT_PeerClose, SelfRegistration::onPeerClose);


	this->registerEndpoint();
}

void SelfRegistration::registerEndpoint()
{
	auto identityType = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","type" }, 0);
	if (identityType == ::service_discovery::EndPointType::EPT_Service_Registry)
	{
		return;
	}

	if (!APie::CtxSingleton::get().yamlNode()["service_registry"])
	{
		return;
	}

	std::string ip = APie::CtxSingleton::get().yamlAs<std::string>({ "service_registry","address" }, "");
	uint16_t port = APie::CtxSingleton::get().yamlAs<uint16_t>({ "service_registry","port_value" }, 0);
	std::string auth = APie::CtxSingleton::get().yamlAs<std::string>({ "service_registry","auth" }, "");
	uint16_t type = APie::CtxSingleton::get().yamlAs<uint16_t>({ "service_registry","type" }, 0);

	auto ptrSelf = this->shared_from_this();
	auto ptrClient = APie::ClientProxy::createClientProxy();
	auto connectCb = [ptrSelf](std::shared_ptr<APie::ClientProxy> self, uint32_t iResult) {
		if (iResult == 0)
		{
			uint32_t type = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","type" }, 0);
			uint32_t id = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","id" }, 0);
			std::string auth = APie::CtxSingleton::get().yamlAs<std::string>({ "identify","auth" }, "");
			std::string ip = APie::CtxSingleton::get().yamlAs<std::string>({ "identify","ip" }, "");
			uint32_t port = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","port" }, 0);
			uint32_t codec_type = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","codec_type" }, 0);

			::service_discovery::MSG_REQUEST_ADD_INSTANCE request;
			request.mutable_instance()->set_type(static_cast<::service_discovery::EndPointType>(type));
			request.mutable_instance()->set_id(id);
			request.mutable_instance()->set_auth(auth);
			request.mutable_instance()->set_ip(ip);
			request.mutable_instance()->set_port(port);
			request.mutable_instance()->set_codec_type(codec_type);
			self->sendMsg(::opcodes::OP_MSG_REQUEST_ADD_INSTANCE, request);

			ptrSelf->setState(APie::SelfRegistration::Registering);
		}
		return true;
	};
	ptrClient->connect(ip, port, static_cast<APie::ProtocolType>(type), connectCb);

	auto heartbeatCb = [](APie::ClientProxy *ptrClient) {
		ptrClient->addHeartbeatTimer(3000);
	};
	ptrClient->setHeartbeatCb(heartbeatCb);
	ptrClient->addHeartbeatTimer(1000);
	ptrClient->addReconnectTimer(1000);
	ptrClient.reset();
}

void SelfRegistration::unregisterEndpoint()
{

}
void SelfRegistration::heartbeat()
{

}

void SelfRegistration::setState(State state)
{
	m_state = state;
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
		if (items.first.type == type)
		{
			result.push_back(items.first);
		}
	}

	return result;
}

std::optional<uint64_t> EndPointMgr::getSerialNum(EndPoint point)
{
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

	m_establishedPoints[point] = state;
}

void EndPointMgr::clear()
{
	this->m_endpoints.clear();
	this->m_establishedPoints.clear();
}


void SelfRegistration::handleRespAddInstance(uint64_t iSerialNum, const ::service_discovery::MSG_RESP_ADD_INSTANCE& response)
{
	std::cout << "iSerialNum:" << iSerialNum << ",response:" << response.DebugString() << std::endl;

	if (response.status_code() != opcodes::StatusCode::SC_Ok)
	{
		return;
	}

	APie::CtxSingleton::get().getEndpoint()->setState(APie::SelfRegistration::Registered);
}

void SelfRegistration::handleNoticeInstance(uint64_t iSerialNum, const ::service_discovery::MSG_NOTICE_INSTANCE& notice)
{
	std::cout << "iSerialNum:" << iSerialNum << ",notice:" << notice.DebugString() << std::endl;

	switch (notice.mode())
	{
	case service_discovery::UM_Full:
	{
		EndPointMgrSingleton::get().clear();
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
}

void SelfRegistration::handleAddRoute(uint64_t iSerialNum, const ::route_register::MSG_REQUEST_ADD_ROUTE& request)
{
	EndPoint point;
	point.type = request.instance().type();
	point.id = request.instance().id();

	::route_register::MSG_RESP_ADD_ROUTE response;

	auto findOpt = EndPointMgrSingleton::get().findEndpoint(point);
	if (!findOpt.has_value())
	{
		response.set_status_code(opcodes::SC_Route_InvalidPoint);
		APie::Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_MSG_RESP_ADD_ROUTE, response);
		return;
	}

	auto auth = findOpt.value().auth();
	if (!auth.empty() && auth != request.instance().auth())
	{
		response.set_status_code(opcodes::SC_Route_AuthError);
		APie::Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_MSG_RESP_ADD_ROUTE, response);
		return;
	}

	response.set_status_code(opcodes::SC_Ok);
	APie::Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_MSG_RESP_ADD_ROUTE, response);

	EndPointMgrSingleton::get().addRoute(point, iSerialNum);
}

void SelfRegistration::onPeerClose(uint64_t topic, ::google::protobuf::Message& msg)
{
	auto& refMsg = dynamic_cast<::pubsub::PEER_CLOSE&>(msg);
	std::cout << "topic:" << topic << ",refMsg:" << refMsg.DebugString() << std::endl;

	uint64_t iSerialNum = refMsg.serial_num();
	auto clientProxy = APie::ClientProxy::findClient(iSerialNum);
	if (clientProxy)
	{
		clientProxy->setHadEstablished(ClientProxy::CONNECT_CLOSE);
		clientProxy->onConnect(refMsg.result());
	}
}

}
