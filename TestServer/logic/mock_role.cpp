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

void MockRole::disableCmdTimer()
{
	this->m_cmdTimer->disableTimer();
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

void MockRole::clearResponseHandler()
{
	m_responseHandler.clear();
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

std::map<std::tuple<uint32_t, uint32_t>, std::vector<uint64_t>>& MockRole::getReplyDelay()
{
	return m_replyDelay;
}

std::map<std::tuple<uint32_t, uint32_t>, std::tuple<uint64_t, uint64_t, uint64_t, uint64_t>>& MockRole::getMergeReplyDelay()
{
	return m_mergeReplyDelay;
}

bool MockRole::hasTimeout(uint64_t iCurMS)
{
	for (const auto& elems : m_pendingResponse)
	{
		if (iCurMS > elems.expired_at_ms)
		{
			return true;
		}
	}

	for (const auto& elems : m_pendingNotify)
	{
		if (iCurMS > elems.expired_at_ms)
		{
			return true;
		}
	}

	return false;
}

uint32_t MockRole::addPendingResponse(uint32_t response, uint32_t request, HandleResponseCB cb, uint32_t timeout)
{
	m_id++;

	auto iCurMs = Ctx::getCurMilliseconds();

	PendingResponse pendingObj;
	pendingObj.id = m_id;
	pendingObj.request_opcode = request;
	pendingObj.request_time = iCurMs;
	pendingObj.response_opcode = response;
	pendingObj.cb = cb;
	pendingObj.timeout = timeout;
	pendingObj.expired_at_ms = iCurMs + timeout;

	m_pendingResponse.push_back(pendingObj);

	return m_id;
}

std::optional<PendingResponse> MockRole::findPendingResponse(uint32_t response)
{
	for (auto& elems : m_pendingResponse)
	{
		if (elems.response_opcode == response)
		{
			return std::make_optional(elems);
		}
	}

	return std::nullopt;
}

void MockRole::removePendingResponseById(uint32_t id)
{
	auto cmp = [id](const PendingResponse& elems) {
		if (elems.id == id)
		{
			return true;
		}

		return false;
	};

	m_pendingResponse.remove_if(cmp);
}

void MockRole::clearPendingResponse()
{
	m_pendingResponse.clear();
}


uint32_t MockRole::addPendingNotify(uint32_t response, HandleResponseCB cb, uint32_t timeout)
{
	m_id++;

	auto iCurMs = Ctx::getCurMilliseconds();

	PendingNotify pendingObj;
	pendingObj.id = m_id;
	pendingObj.response_opcode = response;
	pendingObj.cb = cb;
	pendingObj.timeout = timeout;
	pendingObj.expired_at_ms = iCurMs + timeout;

	m_pendingNotify.push_back(pendingObj);

	return m_id;
}

std::optional<PendingNotify> MockRole::findPendingNotify(uint32_t response)
{
	for (auto& elems : m_pendingNotify)
	{
		if (elems.response_opcode == response)
		{
			return std::make_optional(elems);
		}
	}

	return std::nullopt;
}

void MockRole::removePendingNotifyById(uint32_t id)
{
	auto cmp = [id](const PendingNotify& elems) {
		if (elems.id == id)
		{
			return true;
		}

		return false;
	};

	m_pendingNotify.remove_if(cmp);
}

void MockRole::clearPendingNotify()
{
	m_pendingNotify.clear();
}

void MockRole::handlePendingNotify(uint64_t serialNum, uint32_t opcodes, const std::string& msg)
{
	auto findIte = findPendingNotify(opcodes);
	if (!findIte.has_value())
	{
		return;
	}

	if (findIte.value().cb)
	{
		findIte.value().cb(this, serialNum, opcodes, msg);
	}
	
	removePendingNotifyById(findIte.value().id);
}


void MockRole::handleResponse(uint64_t serialNum, uint32_t opcodes, const std::string& msg)
{
	std::string sMsg = msg;
	do 
	{
		auto typeOpt = getPbNameByOpcode(opcodes);
		if (!typeOpt.has_value())
		{
			sMsg = "******unregisterPbMap******";
			break;
		}

		std::string sType = typeOpt.value();
		auto ptrMsg = Api::PBHandler::createMessage(sType);
		if (ptrMsg == nullptr)
		{
			sMsg = "******createMessageError******";
			break;
		}

		std::shared_ptr<::google::protobuf::Message> newMsg(ptrMsg);
		bool bResult = newMsg->ParseFromString(msg);
		if (!bResult)
		{
			sMsg = "******ParseFromStringError******";
			break;
		}

		sMsg = newMsg->ShortDebugString();
	} while (false);

	std::stringstream ss;
	ss << "traffic/" << m_iRoleId;
	std::string fileName = ss.str();

	ss.str("");
	ss << "recv|iSerialNum:" << serialNum << "|iOpcode:" << opcodes << "|data:" << sMsg;
	ASYNC_PIE_LOG_CUSTOM(fileName.c_str(), PIE_CYCLE_DAY, PIE_DEBUG, "%s", ss.str().c_str());

	this->removeWaitResponse(opcodes);

	auto findIte = findResponseHandler(opcodes);
	if (findIte != nullptr)
	{
		findIte(this, serialNum, opcodes, msg);

		//std::cout << ss.str() << std::endl;
		return;
	}

	//std::cout << ss.str() << std::endl;
}

void MockRole::handlePendingResponse(uint64_t serialNum, uint32_t opcodes, const std::string& msg)
{
	auto findIte = findPendingResponse(opcodes);
	if (!findIte.has_value())
	{
		return;
	}

	auto iCurMs = Ctx::getCurMilliseconds();
	auto iDelay = iCurMs - findIte.value().request_time;

	std::tuple<uint32_t, uint32_t> key = { findIte.value().request_opcode, findIte.value().response_opcode};
	auto ite = m_replyDelay.find(key);
	if (ite == m_replyDelay.end())
	{
		std::vector<uint64_t> delayVec;
		delayVec.push_back(iDelay);

		m_replyDelay[key] = delayVec;
	}
	else
	{
		if (ite->second.size() > 1000)
		{
			uint64_t iMin = 0;
			uint64_t iMax = 0;
			uint64_t iAverage = 0;
			uint64_t iTotal = 0;
			uint64_t iCount = 0;
			for (auto& items : ite->second)
			{
				if (iMin == 0)
				{
					iMin = items;
				}

				if (iMax == 0)
				{
					iMax = items;
				}

				if (iMin > items)
				{
					iMin = items;
				}

				if (iMax < items)
				{
					iMax = items;
				}

				iCount++;

				iTotal += items;
			}

			auto findIte = m_mergeReplyDelay.find(key);
			if (findIte == m_mergeReplyDelay.end())
			{
				std::tuple<uint64_t, uint64_t, uint64_t, uint64_t> mergeElem = { iMin, iMax, iCount, iTotal };
				m_mergeReplyDelay[key] = mergeElem;
			}
			else
			{
				auto prevElem = m_mergeReplyDelay[key];

				uint64_t iCurMin = std::get<0>(prevElem) + iMin;
				uint64_t iCurMax = std::get<1>(prevElem) + iMax;
				uint64_t iCurCount = std::get<2>(prevElem) + iCount;
				uint64_t iCurTotal = std::get<3>(prevElem) + iTotal;

				std::tuple<uint64_t, uint64_t, uint64_t, uint64_t> mergeElem = { iCurMin, iCurMax, iCurCount, iCurTotal };
				m_mergeReplyDelay[key] = mergeElem;
			}

			ite->second.clear();
		}

		ite->second.push_back(iDelay);
	}

	if (findIte.value().cb)
	{
		findIte.value().cb(this, serialNum, opcodes, msg);
	}

	removePendingResponseById(findIte.value().id);
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

	this->addPendingResponse(OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, OP_MSG_REQUEST_ACCOUNT_LOGIN_L);
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
	this->addPendingResponse(OP_MSG_RESPONSE_ECHO, OP_MSG_REQUEST_ECHO);
}

void MockRole::handleLogout(::pubsub::LOGIC_CMD& msg)
{
	TestServerMgrSingleton::get().removeMockRole(m_iRoleId);

	uint64_t iSerialNum = 0;
	if (m_clientProxy)
	{
		iSerialNum = m_clientProxy->getSerialNum();
	}
	this->handlePendingNotify(iSerialNum, 0, "active close");
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


				ptrShared->m_clientRandom = "client";

				::login_msg::MSG_REQUEST_HANDSHAKE_INIT request;
				request.set_client_random(ptrShared->m_clientRandom);
				ptrShared->sendMsg(::APie::OP_MSG_REQUEST_HANDSHAKE_INIT, request);
				ptrShared->addPendingResponse(OP_MSG_RESPONSE_HANDSHAKE_INIT, OP_MSG_REQUEST_HANDSHAKE_INIT);

				ptrShared->m_account_id = response.account_id();
				ptrShared->m_session_key = response.session_key();
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
	this->addPendingResponse(OP_MSG_RESPONSE_HANDSHAKE_ESTABLISHED, OP_MSG_REQUEST_HANDSHAKE_ESTABLISHED);

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
	this->addPendingResponse(OP_MSG_RESPONSE_CLIENT_LOGIN, OP_MSG_REQUEST_CLIENT_LOGIN);

	this->setPauseProcess(false);
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
	ASYNC_PIE_LOG_CUSTOM(fileName.c_str(), PIE_CYCLE_DAY, PIE_DEBUG, "%s", ss.str().c_str());
}

}

