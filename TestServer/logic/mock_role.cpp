#include "mock_role.h"

#include <functional>
#include "test_server.h"

#include "../../SharedDir/opcodes.h"

namespace APie {

MockRole::MockRole(uint64_t iRoleId) :
	m_iRoleId(iRoleId)
{
}

MockRole::~MockRole()
{
	if (this->m_cmdTimer)
	{
		this->m_cmdTimer->disableTimer();
	}

	if (this->m_clientProxy)
	{
		TestServerMgrSingleton::get().removeSerialNum(this->m_clientProxy->getSerialNum());

		this->m_clientProxy->onActiveClose();
	}
}

void MockRole::setUp()
{
	if (m_bInit)
	{
		return;
	}

	m_bInit = true;
	TestServerMgrSingleton::get().addSerialNumRole(this->m_clientProxy->getSerialNum(), m_iRoleId);

	this->addHandler("account_login", std::bind(&MockRole::handleAccountLogin, this, std::placeholders::_1));
	//this->addHandler("login", std::bind(&MockRole::handleLogin, this, std::placeholders::_1));
	this->addHandler("echo", std::bind(&MockRole::handleEcho, this, std::placeholders::_1));
	this->addHandler("logout", std::bind(&MockRole::handleLogout, this, std::placeholders::_1));


	this->addResponseHandler(::APie::OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, &MockRole::handle_MSG_RESPONSE_ACCOUNT_LOGIN_L);

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
	m_target = CT_Login;

	auto cmdCb = [ptrSelf]() mutable {
		auto ptrShared = ptrSelf.lock();
		if (ptrShared)
		{
			ptrShared->processCmd();
			ptrShared->addTimer(100);
		}
	};
	this->m_cmdTimer = APie::CtxSingleton::get().getLogicThread()->dispatcher().createTimer(cmdCb);
	this->addTimer(100);
}

uint64_t MockRole::getRoleId()
{
	return m_iRoleId;
}

void MockRole::processCmd()
{
	if (!m_bInit)
	{
		return;
	}

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
		if (m_bPauseProcess)
		{
			break;
		}

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
		std::cout << "invalid cmd:" << sCmd << std::endl;
		return;
	}
	
	try
	{
		handler(msg);
	}
	catch (std::exception& e)
	{
		std::stringstream ss;
		ss << "Unexpected exception: " << e.what();
		PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "Exception", ss.str().c_str());
	}
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
	auto findIte = m_cmdHandler.find(name);
	if (findIte != m_cmdHandler.end())
	{
		return false;
	}

	m_cmdHandler[name] = cb;
	return true;
}

MockRole::HandlerCb MockRole::findHandler(const std::string& name)
{
	auto findIte = m_cmdHandler.find(name);
	if (findIte == m_cmdHandler.end())
	{
		return nullptr;
	}

	return findIte->second;
}

bool MockRole::addResponseHandler(uint32_t opcodes, HandleResponseCB cb)
{
	auto findIte = m_responseHandler.find(opcodes);
	if (findIte != m_responseHandler.end())
	{
		return false;
	}

	m_responseHandler[opcodes] = cb;
	return true;
}

MockRole::HandleResponseCB MockRole::findResponseHandler(uint32_t opcodes)
{
	auto findIte = m_responseHandler.find(opcodes);
	if (findIte == m_responseHandler.end())
	{
		return nullptr;
	}

	return findIte->second;
}

void MockRole::setPauseProcess(bool flag)
{
	m_bPauseProcess = false;
}

void MockRole::handleResponse(uint64_t serialNum, uint32_t opcodes, const std::string& msg)
{
	auto findIte = findResponseHandler(opcodes);
	if (findIte != nullptr)
	{
		findIte(this, serialNum, opcodes, msg);
		return;
	}


	std::string sData;

	switch (opcodes)
	{
	case ::APie::OP_MSG_RESPONSE_CLIENT_LOGIN:
	{
		::login_msg::MSG_RESPONSE_CLIENT_LOGIN response;
		bool bResult = response.ParseFromString(msg);
		if (!bResult)
		{
			sData = "parse error";
			return;
		}
		
		sData = response.ShortDebugString();
		break;
	}
	case ::APie::OP_MSG_RESPONSE_ECHO:
	{
		::login_msg::MSG_RESPONSE_ECHO response;
		bool bResult = response.ParseFromString(msg);
		if (!bResult)
		{
			sData = "parse error";
			return;
		}

		sData = response.ShortDebugString();
		break;
	}
	default:
	{
		sData = msg;
		break;
	}
	}

	std::stringstream ss;
	ss << "handleResponse|roleId:" << m_iRoleId << "|serialNum:" << serialNum << "|iOpcode:" << opcodes << "|msg:" << sData;
	ASYNC_PIE_LOG("handleResponse/recv", PIE_CYCLE_HOUR, PIE_NOTICE, "%s", ss.str().c_str());

	std::cout << ss.str() << std::endl;
}

std::shared_ptr<MockRole> MockRole::createMockRole(uint64_t iRoleId)
{
	return std::make_shared<MockRole>(iRoleId);
}

void MockRole::handleAccountLogin(::pubsub::LOGIC_CMD& msg)
{
	::login_msg::MSG_REQUEST_ACCOUNT_LOGIN_L request;
	request.set_account_id(m_iRoleId);

	this->sendMsg(::APie::OP_MSG_REQUEST_ACCOUNT_LOGIN_L, request);
}

//void MockRole::handleLogin(::pubsub::LOGIC_CMD& msg)
//{
//	::login_msg::MSG_REQUEST_CLIENT_LOGIN request;
//	request.set_user_id(m_iRoleId);
//	request.set_version(std::stoi(msg.params()[0]));
//	request.set_session_key(msg.params()[1]);
//
//	this->sendMsg(::APie::OP_MSG_REQUEST_CLIENT_LOGIN, request);
//}

void MockRole::handleEcho(::pubsub::LOGIC_CMD& msg)
{
	::login_msg::MSG_REQUEST_ECHO request;
	request.set_value1(std::stoi(msg.params()[0]));
	request.set_value2(msg.params()[1]);

	this->sendMsg(::APie::OP_MSG_REQUEST_ECHO, request);
}

void MockRole::handleLogout(::pubsub::LOGIC_CMD& msg)
{
	TestServerMgrSingleton::get().removeMockRole(m_iRoleId);
}

void MockRole::handle_MSG_RESPONSE_ACCOUNT_LOGIN_L(uint64_t serialNum, uint32_t opcodes, const std::string& msg)
{
	this->setPauseProcess(true);

	std::stringstream ss;
	ss << "handleResponse|roleId:" << m_iRoleId << "|serialNum:" << serialNum << "|iOpcode:" << opcodes;
	

	::login_msg::MSG_RESPONSE_ACCOUNT_LOGIN_L response;
	bool bResult = response.ParseFromString(msg);
	if (!bResult)
	{
		ASYNC_PIE_LOG("handleResponse/recv", PIE_CYCLE_HOUR, PIE_NOTICE, "%s", ss.str().c_str());

		TestServerMgrSingleton::get().removeMockRole(m_iRoleId);
		return;
	}

	ss << "data|" << response.ShortDebugString();
	ASYNC_PIE_LOG("handleResponse/recv", PIE_CYCLE_HOUR, PIE_NOTICE, "%s", ss.str().c_str());

	TestServerMgrSingleton::get().removeSerialNum(this->m_clientProxy->getSerialNum());
	this->m_clientProxy->onActiveClose();


	std::string ip = response.ip();
	uint32_t port = response.port();
	uint16_t type = APie::CtxSingleton::get().yamlAs<uint16_t>({ "clients", "socket_address", "type" }, 0);
	uint32_t maskFlag = APie::CtxSingleton::get().yamlAs<uint16_t>({ "clients", "socket_address", "mask_flag" }, 0);


	m_clientProxy = APie::ClientProxy::createClientProxy();
	std::weak_ptr<MockRole> ptrSelf = this->shared_from_this();
	auto connectCb = [ptrSelf, response](APie::ClientProxy* ptrClient, uint32_t iResult) mutable {
		if (iResult == 0)
		{
			auto ptrShared = ptrSelf.lock();
			if (ptrShared)
			{
				TestServerMgrSingleton::get().addSerialNumRole(ptrShared->m_clientProxy->getSerialNum(), ptrShared->m_iRoleId);
				ptrShared->setPauseProcess(false);


				::login_msg::MSG_REQUEST_CLIENT_LOGIN request;
				request.set_user_id(response.account_id());
				request.set_session_key(response.session_key());

				ptrShared->sendMsg(::APie::OP_MSG_REQUEST_CLIENT_LOGIN, request);
			}
		}
		return true;
	};
	m_clientProxy->connect(ip, port, static_cast<APie::ProtocolType>(type), maskFlag, connectCb);
	m_clientProxy->addReconnectTimer(1000);
	m_target = CT_Gateway;

}

void MockRole::sendMsg(uint32_t iOpcode, const ::google::protobuf::Message& msg)
{
	m_clientProxy->sendMsg(iOpcode, msg);
}

}

