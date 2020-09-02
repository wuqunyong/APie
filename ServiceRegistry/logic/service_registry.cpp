#include "service_registry.h"

namespace APie {

void ServiceRegistry::init()
{
	APie::Api::OpcodeHandlerSingleton::get().server.bind(::opcodes::OP_MSG_REQUEST_ADD_INSTANCE, ServiceRegistry::handleRequestAddInstance, ::service_discovery::MSG_REQUEST_ADD_INSTANCE::default_instance());

	APie::PubSubSingleton::get().subscribe(::pubsub::PT_ServerPeerClose, ServiceRegistry::onServerPeerClose);
}

bool ServiceRegistry::updateInstance(uint64_t iSerialNum, const ::service_discovery::EndPointInstance& instance)
{
	auto curTime = APie::CtxSingleton::get().getNowSeconds();


	EndPoint point;
	point.type = instance.type();
	point.id = instance.id();

	auto findPoint = m_pointMap.find(point);
	if (findPoint != m_pointMap.end())
	{
		return false;
	}

	//add
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

	return true;
}

bool ServiceRegistry::deleteBySerialNum(uint64_t iSerialNum)
{
	bool bResult = false;
	auto findIte = m_registered.find(iSerialNum);
	if (findIte != m_registered.end())
	{
		EndPoint point;
		point.type = findIte->second.instance.type();
		point.id = findIte->second.instance.id();

		m_pointMap.erase(point);
		m_registered.erase(findIte);

		bResult = true;
	}

	return bResult;
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
	std::stringstream ss;
	ss << "iSerialNum:" << iSerialNum << ",request:" << request.ShortDebugString();

	::service_discovery::MSG_RESP_ADD_INSTANCE response;

	auto auth = APie::CtxSingleton::get().identify().auth;
	if (!auth.empty() && auth != request.auth())
	{
		response.set_status_code(opcodes::SC_Discovery_AuthError);
		APie::Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_MSG_RESP_ADD_INSTANCE, response);

		ss << ",auth:error";
		ASYNC_PIE_LOG("SelfRegistration/handleRequestAddInstance", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());
		return;
	}

	bool bResult = ServiceRegistrySingleton::get().updateInstance(iSerialNum, request.instance());
	if (!bResult)
	{
		response.set_status_code(opcodes::SC_Discovery_DuplicateNode);
		APie::Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_MSG_RESP_ADD_INSTANCE, response);

		ss << ",node:duplicate";
		ASYNC_PIE_LOG("SelfRegistration/handleRequestAddInstance", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());
		return;
	}
	
	ASYNC_PIE_LOG("SelfRegistration/handleRequestAddInstance", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());

	response.set_status_code(::opcodes::StatusCode::SC_Ok);
	APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_MSG_RESP_ADD_INSTANCE, response);

	ServiceRegistrySingleton::get().broadcast();
}

void ServiceRegistry::onServerPeerClose(uint64_t topic, ::google::protobuf::Message& msg)
{
	std::stringstream ss;
	auto& refMsg = dynamic_cast<::pubsub::SERVER_PEER_CLOSE&>(msg);
	ss << "topic:" << topic << ",refMsg:" << refMsg.ShortDebugString();
	ASYNC_PIE_LOG("SelfRegistration/onServerPeerClose", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	uint64_t iSerialNum = refMsg.serial_num();
	bool bChanged = ServiceRegistrySingleton::get().deleteBySerialNum(iSerialNum);
	if (bChanged)
	{
		ServiceRegistrySingleton::get().broadcast();
	}
}

}

