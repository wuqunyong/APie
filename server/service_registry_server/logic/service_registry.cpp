#include "service_registry.h"

namespace APie {

std::tuple<uint32_t, std::string> ServiceRegistry::init()
{
	auto bResult = APie::CtxSingleton::get().checkIsValidServerType({ common::EPT_Service_Registry });
	if (!bResult)
	{
		return std::make_tuple(Hook::HookResult::HR_Error, "invalid Type");
	}

	// CMD
	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_LogicCmd, ServiceRegistry::onLogicCommnad);

	LogicCmdHandlerSingleton::get().init();
	LogicCmdHandlerSingleton::get().registerOnCmd("provider", "show_provider", ServiceRegistry::onShowProvider);

	// RPC
	APie::RPC::rpcInit();

	APie::Api::OpcodeHandlerSingleton::get().server.bind(::opcodes::OP_DISCOVERY_MSG_REQUEST_REGISTER_INSTANCE, ServiceRegistry::handleRequestRegisterInstance);
	APie::Api::OpcodeHandlerSingleton::get().server.bind(::opcodes::OP_DISCOVERY_MSG_REQUEST_HEARTBEAT, ServiceRegistry::handleRequestHeartbeat);

	APie::PubSubSingleton::get().subscribe(::pubsub::PT_ServerPeerClose, ServiceRegistry::onServerPeerClose);

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> ServiceRegistry::start()
{
	m_id = "id_" + APie::CtxSingleton::get().launchTime();
	m_serviceTimeout = APie::CtxSingleton::get().yamlAs<uint32_t>({"service_timeout"}, 300);

	auto timerCb = [this]() {
		this->update();
		this->addUpdateTimer(1000);
	};
	this->m_updateTimer = APie::CtxSingleton::get().getLogicThread()->dispatcher().createTimer(timerCb);
	this->addUpdateTimer(1000);

	APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> ServiceRegistry::ready()
{
	std::stringstream ss;
	ss << "Server Ready!";
	std::cout << ss.str() << std::endl;
	ASYNC_PIE_LOG("ServerStatus", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

void ServiceRegistry::exit()
{
	this->disableUpdateTimer();
}

void ServiceRegistry::onShowProvider(::pubsub::LOGIC_CMD& cmd)
{
	std::stringstream ss;
	for (const auto& items : ServiceRegistrySingleton::get().registered())
	{
		ss << "--> " << "addTime:" << items.second.addTime << "|modifiedTime:" << items.second.modifyTime << "|node:" << items.second.instance.ShortDebugString() << std::endl;
	}

	ASYNC_PIE_LOG("show_provider:\n%s", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());
	std::cout << "show_provider:\n" << ss.str() << std::flush;

}

void ServiceRegistry::addUpdateTimer(uint64_t interval)
{
	this->m_updateTimer->enableTimer(std::chrono::milliseconds(interval));
}

void ServiceRegistry::disableUpdateTimer()
{
	this->m_updateTimer->disableTimer();
}

std::map<uint64_t, RegisteredEndPoint>& ServiceRegistry::registered()
{
	return m_registered;
}

void ServiceRegistry::update()
{
	this->checkTimeout();

	if (this->m_status == service_discovery::RS_Learning)
	{
		auto iDuration = APie::CtxSingleton::get().yamlAs<uint32_t>({ "service_learning_duration" }, 60);

		auto iCurTime = APie::CtxSingleton::get().getCurSeconds();
		if (m_iStatusCheckTime == 0)
		{
			m_iStatusCheckTime = iCurTime;
		}

		if (iCurTime > m_iStatusCheckTime + iDuration)
		{
			this->m_status = service_discovery::RS_Forwarding;
			this->broadcast();
		}
	}
}

bool ServiceRegistry::updateInstance(uint64_t iSerialNum, const ::service_discovery::EndPointInstance& instance)
{
	auto curTime = APie::CtxSingleton::get().getCurSeconds();

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


bool ServiceRegistry::updateHeartbeat(uint64_t iSerialNum)
{
	auto curTime = APie::CtxSingleton::get().getCurSeconds();

	auto findIte = m_registered.find(iSerialNum);
	if (findIte != m_registered.end())
	{
		findIte->second.modifyTime = curTime;

		return true;
	}

	return false;
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

void ServiceRegistry::checkTimeout()
{
	auto curTime = APie::CtxSingleton::get().getCurSeconds();

	std::vector<uint64_t> delSerial;
	for (const auto& items : m_registered)
	{
		if (curTime > items.second.modifyTime + m_serviceTimeout)
		{
			delSerial.push_back(items.first);
		}
	}

	bool bChanged = false;
	for (const auto& items : delSerial)
	{
		ServerConnection::sendCloseLocalServer(items);
		bool bTemp = ServiceRegistrySingleton::get().deleteBySerialNum(items);
		if (bTemp)
		{
			bChanged = true;
		}
	}

	if (bChanged)
	{
		ServiceRegistrySingleton::get().broadcast();
	}
}

void ServiceRegistry::broadcast()
{
	m_version++;

	::service_discovery::MSG_NOTICE_INSTANCE notice;
	notice.set_id(m_id);
	notice.set_version(m_version);
	notice.set_status(m_status);
	notice.set_mode(service_discovery::UM_Full);
	for (const auto& items : m_registered)
	{
		auto ptrAdd = notice.add_add_instance();
		*ptrAdd = items.second.instance;
	}

	for (const auto& items : m_registered)
	{
		APie::Network::OutputStream::sendMsg(items.first, ::opcodes::OPCODE_ID::OP_DISCOVERY_MSG_NOTICE_INSTANCE, notice);
	}
}

void ServiceRegistry::onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg)
{
	auto& command = dynamic_cast<::pubsub::LOGIC_CMD&>(msg);
	auto handlerOpt = LogicCmdHandlerSingleton::get().findCb(command.cmd());
	if (!handlerOpt.has_value())
	{
		return;
	}

	handlerOpt.value()(command);
}

void ServiceRegistry::handleRequestRegisterInstance(uint64_t iSerialNum, const ::service_discovery::MSG_REQUEST_REGISTER_INSTANCE& request)
{
	std::stringstream ss;
	ss << "iSerialNum:" << iSerialNum << ",request:" << request.ShortDebugString();

	::service_discovery::MSG_RESP_REGISTER_INSTANCE response;

	auto auth = APie::CtxSingleton::get().identify().auth;
	if (!auth.empty() && auth != request.auth())
	{
		response.set_status_code(opcodes::SC_Discovery_AuthError);
		APie::Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_DISCOVERY_MSG_RESP_REGISTER_INSTANCE, response);

		ss << ",auth:error";
		ASYNC_PIE_LOG("SelfRegistration/handleRequestRegisterInstance", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());
		return;
	}

	bool bResult = ServiceRegistrySingleton::get().updateInstance(iSerialNum, request.instance());
	if (!bResult)
	{
		response.set_status_code(opcodes::SC_Discovery_DuplicateNode);
		APie::Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_DISCOVERY_MSG_RESP_REGISTER_INSTANCE, response);

		ss << ",node:duplicate";
		ASYNC_PIE_LOG("SelfRegistration/handleRequestRegisterInstance", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());
		return;
	}
	
	ASYNC_PIE_LOG("SelfRegistration/handleRequestRegisterInstance", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());

	response.set_status_code(::opcodes::StatusCode::SC_Ok);
	APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_DISCOVERY_MSG_RESP_REGISTER_INSTANCE, response);

	ServiceRegistrySingleton::get().broadcast();
}

void ServiceRegistry::handleRequestHeartbeat(uint64_t iSerialNum, const ::service_discovery::MSG_REQUEST_HEARTBEAT& request)
{
	std::stringstream ss;
	ss << "iSerialNum:" << iSerialNum << ",request:" << request.ShortDebugString();

	::service_discovery::MSG_RESP_HEARTBEAT response;
	response.set_status_code(opcodes::SC_Ok);

	bool bResult = ServiceRegistrySingleton::get().updateHeartbeat(iSerialNum);
	if (!bResult)
	{
		response.set_status_code(opcodes::SC_Discovery_Unregistered);
		APie::Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_DISCOVERY_MSG_RESP_HEARTBEAT, response);

		ss << ",node:Unregistered";
		ASYNC_PIE_LOG("SelfRegistration/handleRequestHeartbeat", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());
		return;
	}

	APie::Network::OutputStream::sendMsg(iSerialNum, opcodes::OP_DISCOVERY_MSG_RESP_HEARTBEAT, response);
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

