#include "service_registry.h"

namespace APie {

void ServiceRegistry::init()
{
	APie::Api::OpcodeHandlerSingleton::get().server.bind(::opcodes::OPCODE_ID::OP_MSG_REQUEST_ADD_INSTANCE, ServiceRegistry::handleRequestAddInstance, ::service_discovery::MSG_REQUEST_ADD_INSTANCE::default_instance());

	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_ServerPeerClose, ServiceRegistry::onServerPeerClose);
}

void ServiceRegistry::updateInstance(uint64_t iSerialNum, const ::service_discovery::EndPointInstance& instance)
{
	auto curTime = APie::CtxSingleton::get().getNowSeconds();

	auto findIte = m_registered.find(iSerialNum);
	if (findIte == m_registered.end())
	{
		RegisteredEndPoint endPoint;
		endPoint.addTime = curTime;
		endPoint.modifyTime = curTime;
		endPoint.instance = instance;

		m_registered[iSerialNum] = endPoint;
	}
	else
	{
		findIte->second.modifyTime = curTime;
		findIte->second.instance = instance;
	}
}

void ServiceRegistry::handleRequestAddInstance(uint64_t iSerialNum, const ::service_discovery::MSG_REQUEST_ADD_INSTANCE& request)
{
	ServiceRegistrySingleton::get().updateInstance(iSerialNum, request.instance());

	::service_discovery::MSG_RESP_ADD_INSTANCE response;
	response.set_status_code(::opcodes::StatusCode::SC_Ok);
	APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_MSG_RESP_ADD_INSTANCE, response);
}

void ServiceRegistry::onServerPeerClose(uint64_t topic, ::google::protobuf::Message& msg)
{
	auto& refMsg = dynamic_cast<::pubsub::SERVER_PEER_CLOSE&>(msg);
	std::cout << "topic:" << topic << ",refMsg:" << refMsg.DebugString() << std::endl;
}

}

