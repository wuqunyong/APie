#include "mock_role.h"
#include "../../PBMsg/login_msg.pb.h"
#include <functional>
#include "test_server.h"

namespace APie {

MockRole::MockRole(uint64_t iRoleId) :
	m_iRoleId(iRoleId)
{
}

MockRole::~MockRole()
{
	if (this->m_clientProxy)
	{
		TestServerMgrSingleton::get().removeSerialNum(this->m_clientProxy->getSerialNum());

		this->m_clientProxy->onActiveClose();
	}
}

void MockRole::setUp()
{
	TestServerMgrSingleton::get().addSerialNumRole(this->m_clientProxy->getSerialNum(), m_iRoleId);

	this->addHandler("login", std::bind(&MockRole::handleLogin, this, std::placeholders::_1));
	this->addHandler("logout", std::bind(&MockRole::handleLogout, this, std::placeholders::_1));

	this->processCmd();
}

void MockRole::tearDown()
{

}

void MockRole::start()
{
	std::string ip = APie::CtxSingleton::get().yamlAs<std::string>({ "clients", "socket_address", "address" }, "");
	uint16_t port = APie::CtxSingleton::get().yamlAs<uint16_t>({ "clients", "socket_address", "port_value" }, 0);
	uint16_t type = APie::CtxSingleton::get().yamlAs<uint16_t>({ "clients", "socket_address", "type" }, 0);
	uint32_t maskFlag = APie::CtxSingleton::get().yamlAs<uint16_t>({ "clients", "socket_address", "mask_flag" }, 0);

	m_clientProxy = APie::ClientProxy::createClientProxy();

	std::weak_ptr<MockRole> ptrSelf = this->shared_from_this();

	auto connectCb = [ptrSelf](APie::ClientProxy* ptrClient, uint32_t iResult) mutable {
		if (iResult == 0)
		{
			auto ptrShared = ptrSelf.lock();
			if (ptrShared)
			{
				ptrShared->setUp();
			}
		}
		return true;
	};
	m_clientProxy->connect(ip, port, static_cast<APie::ProtocolType>(type), maskFlag, connectCb);
	m_clientProxy->addReconnectTimer(1000);

	auto cmdCb = [ptrSelf]() mutable {
		auto ptrShared = ptrSelf.lock();
		if (ptrShared)
		{
			ptrShared->processCmd();
			ptrShared->addTimer(1000);
		}
	};
	this->m_cmdTimer = APie::CtxSingleton::get().getLogicThread()->dispatcher().createTimer(cmdCb);
	this->addTimer(1000);
}

uint64_t MockRole::getRoleId()
{
	return m_iRoleId;
}

void MockRole::processCmd()
{
	if (!m_clientProxy)
	{
		return;
	}

	if (!m_clientProxy->isConnectted())
	{
		return;
	}

	if (m_configCmd.empty())
	{
		return;
	}

	while (m_iCurIndex < m_configCmd.size())
	{
		auto iOldIndex = m_iCurIndex;

		auto& msg = m_configCmd[m_iCurIndex];
		this->handleMsg(msg);

		if (iOldIndex != m_iCurIndex)
		{
			break;
		}

		m_iCurIndex++;
	}
}

void MockRole::addTimer(uint64_t interval)
{
	this->m_cmdTimer->enableTimer(std::chrono::milliseconds(interval));
}

void MockRole::handleMsg(::pubsub::LOGIC_CMD& msg)
{
	auto sCmd = msg.cmd();
	auto handler = this->findHandler(sCmd);
	if (handler == nullptr)
	{
		return;
	}

	handler(msg);
}

void MockRole::clearMsg()
{
	m_configCmd.clear();
	m_iCurIndex = 0;
}

void MockRole::pushMsg(::pubsub::LOGIC_CMD& msg)
{
	m_configCmd.push_back(msg);
}

bool MockRole::addHandler(const std::string& name, HandlerCb cb)
{
	auto findIte = m_cmdHander.find(name);
	if (findIte != m_cmdHander.end())
	{
		return false;
	}

	m_cmdHander[name] = cb;
	return true;
}

MockRole::HandlerCb MockRole::findHandler(const std::string& name)
{
	auto findIte = m_cmdHander.find(name);
	if (findIte == m_cmdHander.end())
	{
		return nullptr;
	}

	return findIte->second;
}

std::shared_ptr<MockRole> MockRole::createMockRole(uint64_t iRoleId)
{
	return std::make_shared<MockRole>(iRoleId);
}

void MockRole::handleLogin(::pubsub::LOGIC_CMD& msg)
{

}

void MockRole::handleLogout(::pubsub::LOGIC_CMD& msg)
{
	TestServerMgrSingleton::get().removeMockRole(m_iRoleId);
}

}

