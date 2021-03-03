#include "mock_role.h"

#include <functional>
#include "test_server.h"

#include "../../SharedDir/opcodes.h"
#include "../../LibAPie/common/file.h"

namespace APie {

std::map<uint32_t, std::string> MockRole::s_pbReflect;

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
	this->addResponseHandler(::APie::OP_MSG_RESPONSE_HANDSHAKE_INIT, &MockRole::handle_MSG_RESPONSE_HANDSHAKE_INIT);
	this->addResponseHandler(::APie::OP_MSG_RESPONSE_HANDSHAKE_ESTABLISHED, &MockRole::handle_MSG_RESPONSE_HANDSHAKE_ESTABLISHED);


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

		if (!m_waitResponse.empty())
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
		ss << "roleId:" << m_iRoleId << "|Unexpected exception: " << e.what();
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

void MockRole::addWaitResponse(uint32_t iOpcode, uint32_t iNeedCheck)
{
	m_waitResponse[iOpcode] = iNeedCheck;
}

void MockRole::removeWaitResponse(uint32_t iOpcode)
{
	m_waitResponse.erase(iOpcode);
}

void MockRole::handleResponse(uint64_t serialNum, uint32_t opcodes, const std::string& msg)
{
	std::string sMsg = msg;
	do 
	{
		auto typeOpt = getPbNameByOpcode(opcodes);
		if (!typeOpt.has_value())
		{
			break;
		}

		std::string sType = typeOpt.value();
		auto ptrMsg = Api::PBHandler::createMessage(sType);
		if (ptrMsg == nullptr)
		{
			break;
		}

		std::shared_ptr<::google::protobuf::Message> newMsg(ptrMsg);
		bool bResult = newMsg->ParseFromString(msg);
		if (!bResult)
		{
			break;
		}

		sMsg = newMsg->ShortDebugString();
	} while (false);

	std::stringstream ss;
	ss << "traffic/" << m_iRoleId;
	std::string fileName = ss.str();

	ss.str("");
	ss << "recv|iSerialNum:" << serialNum << "|iOpcode:" << opcodes << "|data:" << sMsg;
	ASYNC_PIE_LOG_CUSTOM(fileName.c_str(), PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());

	this->removeWaitResponse(opcodes);

	auto findIte = findResponseHandler(opcodes);
	if (findIte != nullptr)
	{
		findIte(this, serialNum, opcodes, msg);
		return;
	}

	std::cout << ss.str() << std::endl;
}

std::shared_ptr<MockRole> MockRole::createMockRole(uint64_t iRoleId)
{
	return std::make_shared<MockRole>(iRoleId);
}

bool MockRole::registerPbOpcodeName(uint32_t iOpcode, const std::string& sName)
{
	auto findIte = s_pbReflect.find(iOpcode);
	if (findIte != s_pbReflect.end())
	{
		return false;
	}

	s_pbReflect[iOpcode] = sName;
	return true;
}

std::optional<std::string> MockRole::getPbNameByOpcode(uint32_t iOpcode)
{
	auto findIte = s_pbReflect.find(iOpcode);
	if (findIte == s_pbReflect.end())
	{
		return std::nullopt;
	}

	return findIte->second;
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
				ptrShared->setPauseProcess(true);


				ptrShared->m_clientRandom = "client";

				::login_msg::MSG_REQUEST_HANDSHAKE_INIT request;
				request.set_client_random(ptrShared->m_clientRandom);
				ptrShared->sendMsg(::APie::OP_MSG_REQUEST_HANDSHAKE_INIT, request);

				ptrShared->m_account_id = response.account_id();
				ptrShared->m_session_key = response.session_key();

				//::login_msg::MSG_REQUEST_CLIENT_LOGIN request;
				//request.set_user_id(response.account_id());
				//request.set_session_key(response.session_key());

				//ptrShared->sendMsg(::APie::OP_MSG_REQUEST_CLIENT_LOGIN, request);
			}
		}
		return true;
	};
	m_clientProxy->connect(ip, port, static_cast<APie::ProtocolType>(type), maskFlag, connectCb);
	m_clientProxy->addReconnectTimer(1000);
	m_target = CT_Gateway;

}

void MockRole::handle_MSG_RESPONSE_HANDSHAKE_INIT(uint64_t serialNum, uint32_t opcodes, const std::string& msg)
{
	std::stringstream ss;
	ss << "handle_MSG_RESPONSE_HANDSHAKE_INIT|roleId:" << m_iRoleId << "|serialNum:" << serialNum << "|iOpcode:" << opcodes;


	::login_msg::MSG_RESPONSE_HANDSHAKE_INIT response;
	bool bResult = response.ParseFromString(msg);
	if (!bResult)
	{
		ASYNC_PIE_LOG("handleResponse/recv", PIE_CYCLE_HOUR, PIE_NOTICE, "%s", ss.str().c_str());

		TestServerMgrSingleton::get().removeMockRole(m_iRoleId);
		return;
	}

	std::string plainMsg("client_key");
	std::string encryptedMsg;

	std::string content = response.public_key();

	const std::vector<uint8_t> keyDer(content.begin(), content.end());

	BIO* ptrBIO = BIO_new_mem_buf(&content[0], content.size());
	std::unique_ptr<BIO, decltype(BIO_free)*> bio(ptrBIO, BIO_free);

	RSA* ptrRSA = PEM_read_bio_RSA_PUBKEY(bio.get(), NULL, NULL, NULL);
	std::unique_ptr<RSA, decltype(RSA_free)*> rsa(ptrRSA, RSA_free);

	APie::Crypto::RSAUtilitySingleton::get().encryptByPub(rsa.get(), plainMsg, &encryptedMsg);


	this->m_sharedKey = this->m_clientRandom + response.server_random() + plainMsg;


	::login_msg::MSG_REQUEST_HANDSHAKE_ESTABLISHED request;
	request.set_encrypted_key(encryptedMsg);
	this->sendMsg(::APie::OP_MSG_REQUEST_HANDSHAKE_ESTABLISHED, request);


	APie::SetClientSessionAttr *ptr = new APie::SetClientSessionAttr;
	ptr->iSerialNum = this->m_clientProxy->getSerialNum();
	ptr->optKey = this->m_sharedKey;

	Command cmd;
	cmd.type = Command::set_client_session_attr;
	cmd.args.set_client_session_attr.ptrData = ptr;
	Network::OutputStream::sendCommand(ConnetionType::CT_CLIENT, ptr->iSerialNum, cmd);

}

void MockRole::handle_MSG_RESPONSE_HANDSHAKE_ESTABLISHED(uint64_t serialNum, uint32_t opcodes, const std::string& msg)
{
	std::stringstream ss;
	ss << "handle_MSG_RESPONSE_HANDSHAKE_ESTABLISHED|roleId:" << m_iRoleId << "|serialNum:" << serialNum << "|iOpcode:" << opcodes;


	::login_msg::MSG_RESPONSE_HANDSHAKE_ESTABLISHED response;
	bool bResult = response.ParseFromString(msg);
	if (!bResult)
	{
		ASYNC_PIE_LOG("handleResponse/recv", PIE_CYCLE_HOUR, PIE_NOTICE, "%s", ss.str().c_str());

		TestServerMgrSingleton::get().removeMockRole(m_iRoleId);
		return;
	}

	::login_msg::MSG_REQUEST_CLIENT_LOGIN request;
	request.set_user_id(this->m_account_id);
	request.set_session_key(this->m_session_key);
	this->sendMsg(::APie::OP_MSG_REQUEST_CLIENT_LOGIN, request);

	this->addWaitResponse(::APie::OP_MSG_RESPONSE_CLIENT_LOGIN, 1);
}

void MockRole::sendMsg(uint32_t iOpcode, const ::google::protobuf::Message& msg)
{
	m_clientProxy->sendMsg(iOpcode, msg);

	auto iSerialNum = m_clientProxy->getSerialNum();

	std::stringstream ss;
	ss << "traffic/" << m_iRoleId;
	std::string fileName = ss.str();

	ss.str("");
	ss << "send|iSerialNum:" << iSerialNum << "|iOpcode:" << iOpcode << "|data:" << msg.ShortDebugString();
	ASYNC_PIE_LOG_CUSTOM(fileName.c_str(), PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());
}

}

