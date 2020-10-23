#include "mock_role.h"
#include "../../PBMsg/login_msg.pb.h"

namespace APie {

MockRole::MockRole(uint64_t iRoleId) :
	m_iRoleId(iRoleId)
{
	std::string ip = APie::CtxSingleton::get().yamlAs<std::string>({ "clients", "socket_address", "address" }, "");
	uint16_t port = APie::CtxSingleton::get().yamlAs<uint16_t>({ "clients", "socket_address", "port_value" }, 0);
	uint16_t type = APie::CtxSingleton::get().yamlAs<uint16_t>({ "clients", "socket_address", "type" }, 0);

	m_clientProxy = APie::ClientProxy::createClientProxy();

	std::weak_ptr<MockRole> ptrSelf = this->shared_from_this();

	auto connectCb = [ptrSelf](APie::ClientProxy* ptrClient, uint32_t iResult) mutable {
		if (iResult == 0)
		{
			auto self = ptrSelf.lock();
			if (self)
			{
				self->setUp();
			}
		}
		return true;
	};
	m_clientProxy->connect(ip, port, static_cast<APie::ProtocolType>(type), connectCb);
	m_clientProxy->addReconnectTimer(1000);

	auto cmdCb = [ptrSelf]() mutable {
		auto self = ptrSelf.lock();
		if (self)
		{
			self->processCmd();
			self->addTimer(1000);
		}
	};
	this->m_cmdTimer = APie::CtxSingleton::get().getLogicThread()->dispatcher().createTimer(cmdCb);
	//this->addTimer(1000);
}

void MockRole::setUp()
{

}

void MockRole::tearDown()
{

}

void MockRole::processCmd()
{

}

void MockRole::addTimer(uint64_t interval)
{
	this->m_cmdTimer->enableTimer(std::chrono::milliseconds(interval));
}

std::shared_ptr<MockRole> MockRole::createMockRole(uint64_t iRoleId)
{
	return std::make_shared<MockRole>(iRoleId);
}

}

