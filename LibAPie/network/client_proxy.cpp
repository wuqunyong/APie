#include "client_proxy.h"

#include <assert.h>
#include <iostream>
#include <sstream>


#include "Ctx.h"
#include "Command.h"
#include "logger.h"
#include "Ctx.h"
#include "output_stream.h"
#include "../event/dispatcher_impl.h"

using namespace APie;

std::mutex ClientProxy::m_sync;
std::map<uint64_t, std::shared_ptr<ClientProxy>> ClientProxy::m_clientProxy;

ClientProxy::ClientProxy() 
{
	this->m_tag = 0xbadcafe0;
	this->m_curSerialNum = 0;
	this->m_port = 0;
	this->m_codecType = ProtocolType::PT_PB;

	this->m_hadEstablished = CONNECT_CLOSE;
}

ClientProxy::~ClientProxy()
{
	this->m_tag = 0xdeadbeef;
}

bool ClientProxy::checkTag()
{
	return this->m_tag == 0xbadcafe0;
}

int ClientProxy::connect(const std::string& ip, uint16_t port, ProtocolType type, HandleConnectCB cb)
{
	if (this->m_curSerialNum != 0)
	{
		return 1;
	}

	this->m_curSerialNum = generatorId();
	this->m_ip = ip;
	this->m_port = port;
	this->m_codecType = type;
	this->m_cb = cb;

	auto ptrProxy = shared_from_this();
	ClientProxy::registerClient(ptrProxy);

	auto *ptr = new DialParameters;
	if (NULL == ptr)
	{
		return 2;
	}
	ptr->sIp = this->m_ip;
	ptr->iPort = this->m_port;
	ptr->iCodecType = this->m_codecType;
	ptr->iCurSerialNum = this->m_curSerialNum;

	Command cmd;
	cmd.type = Command::dial;
	cmd.args.dial.ptrData = ptr;

	auto ptrIOThread = APie::CtxSingleton::get().chooseIOThread();
	if (ptrIOThread == NULL)
	{
		return 3;
	}
	ptrIOThread->push(cmd);

	std::stringstream ss;
	ss << "send|SerialNum:" << this->m_curSerialNum << ",ip:" << this->m_ip << ",port:" << this->m_port;
	ASYNC_PIE_LOG("ClientProxy/connect", PIE_CYCLE_HOUR, PIE_NOTICE, ss.str().c_str());

	return 0;
}

uint64_t ClientProxy::getSerialNum()
{
	return this->m_curSerialNum;
}

std::string ClientProxy::getCurSerialNumToStr()
{
	std::stringstream ss;
	ss << this->m_curSerialNum;

	return ss.str();
}

ProtocolType ClientProxy::getCodecType()
{
	return this->m_codecType;
}

std::string ClientProxy::getHosts()
{
	std::stringstream ss;
	ss << this->m_ip << ":" << this->m_port;

	return ss.str();
}

int32_t ClientProxy::sendMsg(uint32_t iOpcode, const ::google::protobuf::Message& msg)
{
	if (this->m_hadEstablished != CONNECT_ESTABLISHED)
	{
		return -1;
	}

	APie::Network::OutputStream::sendMsg(APie::Network::ConnetionType::CT_CLIENT, this->m_curSerialNum, iOpcode, msg);
	return 0;
}


void ClientProxy::onConnect(uint32_t iResult)
{
	std::stringstream ss;
	ss << "recv|SerialNum:" << this->m_curSerialNum << ",ip:" << this->m_ip << ",port:" << this->m_port << ",iResult:" << iResult;
	ASYNC_PIE_LOG("ClientProxy/onConnect", PIE_CYCLE_HOUR, PIE_NOTICE, ss.str().c_str());

	if (iResult == 0)
	{
		this->m_hadEstablished = CONNECT_ESTABLISHED;
	}

	bool bContinue = false;
	if (m_cb)
	{
		bContinue = m_cb(shared_from_this(), iResult);
	}

	if (iResult != 0)
	{
		if (bContinue)
		{
			return;
		}

		this->close();
	}
}

void ClientProxy::onPassiveClose(uint32_t iResult, const std::string& sInfo, uint32_t iActiveClose)
{
	std::stringstream ss;
	ss << "recv|SerialNum:" << this->m_curSerialNum << ",ip:" << this->m_ip << ",port:" << this->m_port 
		<< ",iResult:" << iResult << ",sInfo:" << sInfo << ",iActiveClose:" << iActiveClose;
	ASYNC_PIE_LOG("ClientProxy/onPassiveClose", PIE_CYCLE_HOUR, PIE_NOTICE, ss.str().c_str());

	this->m_hadEstablished = CONNECT_CLOSE;

	this->close();
}

void ClientProxy::sendClose()
{
	//APie::ClosePeerNode *itemPtr = new APie::ClosePeerNode;
	//itemPtr->iSerialNum = this->m_curSerialNum;
	//APie::IOThread::deliverCloseQueue(itemPtr);
}

void ClientProxy::close()
{
	std::stringstream ss;
	ss << "close|SerialNum:" << this->m_curSerialNum << ",ip:" << this->m_ip << ",port:" << this->m_port;
	ASYNC_PIE_LOG("ClientProxy/close", PIE_CYCLE_HOUR, PIE_NOTICE, ss.str().c_str());

	ClientProxy::unregisterClient(this->m_curSerialNum);
}

uint64_t ClientProxy::generatorId()
{
	uint64_t iSerialNum = APie::Event::DispatcherImpl::generatorSerialNum();
	return iSerialNum;
}

void ClientProxy::onRecvPackage(uint64_t iSerialNum, ::google::protobuf::Message* ptrMsg)
{
	std::stringstream ss;
	ss << "iSerialNum:" << iSerialNum << ",Message:" << ptrMsg->DebugString();
	ASYNC_PIE_LOG("ClientProxy/onRecvPackage", PIE_CYCLE_HOUR, PIE_NOTICE, ss.str().c_str());
}

bool ClientProxy::registerClient(std::shared_ptr<ClientProxy> ptrClient)
{
	std::lock_guard<std::mutex> guard(m_sync);
	m_clientProxy[ptrClient->getSerialNum()] = ptrClient;
	return true;
}

void ClientProxy::unregisterClient(uint64_t iSerialNum)
{
	std::lock_guard<std::mutex> guard(m_sync);
	m_clientProxy.erase(iSerialNum);
}

std::shared_ptr<ClientProxy> ClientProxy::findClient(uint64_t iSerialNum)
{
	std::lock_guard<std::mutex> guard(m_sync);
	auto findIte = m_clientProxy.find(iSerialNum);
	if (findIte == m_clientProxy.end())
	{
		return nullptr;
	}

	return findIte->second;
}

std::shared_ptr<ClientProxy> ClientProxy::createClientProxy()
{
	return std::make_shared<ClientProxy>();
}