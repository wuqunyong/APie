#include "service_registry.h"

namespace APie {

void ServiceRegistry::init()
{
	APie::Api::OpcodeHandlerSingleton::get().server.bind(::opcodes::OP_MSG_REQUEST_ADD_INSTANCE, ServiceRegistry::handleRequestAddInstance, ::service_discovery::MSG_REQUEST_ADD_INSTANCE::default_instance());

	APie::PubSubSingleton::get().subscribe(::pubsub::PT_ServerPeerClose, ServiceRegistry::onServerPeerClose);
}

void ServiceRegistry::updateInstance(uint64_t iSerialNum, const ::service_discovery::EndPointInstance& instance)
{
	auto curTime = APie::CtxSingleton::get().getNowSeconds();

	EndPoint point;
	point.type = instance.type();
	point.id = instance.id();
	auto findPoint = m_pointMap.find(point);
	if (findPoint != m_pointMap.end())
	{
		m_registered.erase(findPoint->second);
		m_pointMap.erase(findPoint);
	}
	m_pointMap[point] = iSerialNum;


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

void ServiceRegistry::deleteBySerialNum(uint64_t iSerialNum)
{
	auto findIte = m_registered.find(iSerialNum);
	if (findIte != m_registered.end())
	{
		EndPoint point;
		point.type = findIte->second.instance.type();
		point.id = findIte->second.instance.id();

		m_pointMap.erase(point);
		m_registered.erase(findIte);
	}
}

void ServiceRegistry::broadcast()
{
	::service_discovery::MSG_NOTICE_INSTANCE notice;
	notice.set_mode(service_discovery::UM_Full);
	for (const auto& items : m_registered)
	{
		auto ptrAdd = notice.add_add_instance();
		*ptrAdd = items.second.instance;
	}

	for (const auto& items : m_registered)
	{
		APie::Network::OutputStream::sendMsg(items.first, ::opcodes::OPCODE_ID::OP_MSG_NOTICE_INSTANCE, notice);
	}
}

void ServiceRegistry::handleRequestAddInstance(uint64_t iSerialNum, const ::service_discovery::MSG_REQUEST_ADD_INSTANCE& request)
{
	std::cout << "iSerialNum:" << iSerialNum << ",request:" << request.DebugString() << std::endl;

	ServiceRegistrySingleton::get().updateInstance(iSerialNum, request.instance());

	::service_discovery::MSG_RESP_ADD_INSTANCE response;
	response.set_status_code(::opcodes::StatusCode::SC_Ok);
	APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_MSG_RESP_ADD_INSTANCE, response);

	ServiceRegistrySingleton::get().broadcast();
}

void ServiceRegistry::onServerPeerClose(uint64_t topic, ::google::protobuf::Message& msg)
{
	auto& refMsg = dynamic_cast<::pubsub::SERVER_PEER_CLOSE&>(msg);
	std::cout << "topic:" << topic << ",refMsg:" << refMsg.DebugString() << std::endl;

	uint64_t iSerialNum = refMsg.serial_num();
	ServiceRegistrySingleton::get().deleteBySerialNum(iSerialNum);

	ServiceRegistrySingleton::get().broadcast();
}

}

