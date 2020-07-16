#include "end_point.h"

#include "Ctx.h"
#include "client_proxy.h"

#include "../../../PBMsg/opcodes.pb.h"

namespace APie{

void SelfRegistration::init()
{

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

	auto ptrClient = APie::ClientProxy::createClientProxy();
	auto connectCb = [](std::shared_ptr<APie::ClientProxy> self, uint32_t iResult) {
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

			self->addReconnectTimer(3000);
		}
		return true;
	};
	ptrClient->connect(ip, port, static_cast<APie::ProtocolType>(type), connectCb);

	auto heartbeatCb = [](APie::ClientProxy *ptrClient) {
		std::cout << "curTime:" << time(NULL) << std::endl;
		ptrClient->addHeartbeatTimer(3000);
	};
	ptrClient->setHeartbeatCb(heartbeatCb);
	ptrClient->addHeartbeatTimer(1000);
	ptrClient.reset();
}

void SelfRegistration::unregisterEndpoint()
{

}
void SelfRegistration::heartbeat()
{

}


int EndPointMgr::registerEndpoint(::service_discovery::EndPointInstance instance)
{
	return 0;
}
void EndPointMgr::unregisterEndpoint(EndPoint point)
{

}
std::optional<::service_discovery::EndPointInstance> EndPointMgr::findEndpoint(EndPoint point)
{
	return std::nullopt;
}

}
