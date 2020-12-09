#include "client_connection.h"

#include <sstream>
#include <iostream>

#include "ctx.h"

#include "../serialization/protocol_head.h"
#include "logger.h"
#include "../api/pb_handler.h"
#include "address.h"
#include "../decompressor/lz4_decompressor_impl.h"

static const unsigned int MAX_MESSAGE_LENGTH = 16*1024*1024;
static const unsigned int HTTP_BUF_LEN = 8192;

APie::ClientConnection::ClientConnection(integer_t iSerialNum, bufferevent *bev, std::string address, short port, ProtocolType type, uint32_t threadId)
{
	this->iSerialNum = iSerialNum;
	this->bev = bev;
	this->iType = type;
	this->iThreadId = threadId;

	this->SetConnectTo(address,port);

	this->iLocalPort = 0;

	evutil_socket_t fd = bufferevent_getfd(bev);
	struct sockaddr_in addr;
	memset (&addr, 0, sizeof (addr));
	addr.sin_family = AF_INET;

#ifdef WIN32
	int addrlen = sizeof (addr);
#else
	socklen_t addrlen = sizeof (addr);
#endif
	//  Retrieve local port connect is bound to (into addr).
	getsockname(fd, (struct sockaddr*) &addr, &addrlen);

	//this->sLocalAddress = inet_ntoa(addr.sin_addr);

	char buf[100] = { '\0', };
	if (::inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf)) != NULL)
	{
		this->sLocalAddress = buf;
	}

	this->iLocalPort = ntohs(addr.sin_port);

	this->decoder.setConnectSession(this);
}

uint64_t APie::ClientConnection::getSerialNum()
{
	return this->iSerialNum;
}

uint32_t APie::ClientConnection::getTId()
{
	return this->iThreadId;
}

void APie::ClientConnection::close(std::string sInfo, int iCode, int iActive)
{
	std::stringstream ss;
	ss << "close|iSerialNum:" << this->iSerialNum 
		<< "|ip:" << this->sLocalAddress << ":" << this->iLocalPort << " -> " << "peerIp:"<< this->sListenAddress << ":" << this->iListenPort 
		<< "|reason:" << sInfo;
	ASYNC_PIE_LOG("ClientConnection/close", PIE_CYCLE_HOUR, PIE_NOTICE, ss.str().c_str());

	this->sendCloseCmd(iCode, sInfo, iActive);

	APie::Event::DispatcherImpl::delClientConnection(this->iSerialNum);
}

void APie::ClientConnection::sendCloseCmd(uint32_t iResult, const std::string& sInfo, uint32_t iActive)
{
	Command cmd;
	cmd.type = Command::client_peer_close;
	cmd.args.client_peer_close.ptrData = new ClientPeerClose();
	cmd.args.client_peer_close.ptrData->iResult = iResult;
	cmd.args.client_peer_close.ptrData->iSerialNum = this->iSerialNum;
	cmd.args.client_peer_close.ptrData->sInfo = sInfo;
	cmd.args.client_peer_close.ptrData->iActive = iActive;
	APie::CtxSingleton::get().getLogicThread()->push(cmd);
}

void APie::ClientConnection::sendConnectResultCmd(uint32_t iResult)
{
	Command cmd;
	cmd.type = Command::dial_result;
	cmd.args.dial_result.ptrData = new DialResult();
	cmd.args.dial_result.ptrData->iResult = iResult;
	cmd.args.dial_result.ptrData->iSerialNum = this->iSerialNum;

	if (this->bev)
	{
		auto iFd = bufferevent_getfd(this->bev);
		auto ptrAddr = Network::addressFromFd(iFd);
		if (ptrAddr != nullptr)
		{
			auto ip = Network::makeFriendlyAddress(*ptrAddr);
			cmd.args.dial_result.ptrData->sLocalIp = ip;
		}
	}

	APie::CtxSingleton::get().getLogicThread()->push(cmd);
}

APie::ClientConnection::~ClientConnection()
{
	if (this->bev != NULL)
	{
		bufferevent_free(this->bev);
		this->bev = NULL;
	}
}

void APie::ClientConnection::readHttp()
{
	char buf[HTTP_BUF_LEN] = { 0 };

	while (true)
	{
		struct evbuffer *input = bufferevent_get_input(this->bev);
		size_t len = evbuffer_get_length(input);
		if (len == 0)
		{
			return;
		}

		size_t minLen = (len > HTTP_BUF_LEN) ? HTTP_BUF_LEN : len;
		evbuffer_remove(input, buf, minLen);

		int result = decoder.execute(buf, minLen);
		if (result != 0)
		{
			return;
		}
	}
}

void APie::ClientConnection::readPB()
{
	while (true)
	{
		struct evbuffer *input = bufferevent_get_input(this->bev);
		size_t len = evbuffer_get_length(input);

		ProtocolHead head;
		size_t iHeadLen = sizeof(ProtocolHead);
		if (len < iHeadLen)
		{
			return;
		}

		size_t iRecvLen = evbuffer_copyout(input, &head, iHeadLen);
		if (iRecvLen < iHeadLen)
		{
			return; // Message incomplete. Waiting for more bytes.
		}

		uint32_t iBodyLen = head.iBodyLen;
		if (iBodyLen > MAX_MESSAGE_LENGTH)
		{
			// Avoid illegal data (too large message) crashing this client.
			std::stringstream ss;
			ss << "active|" << "Message too large: " << iBodyLen << "|Connection closed.";
			this->close(ss.str());
			return;
		}

		if (evbuffer_get_length(input) < iHeadLen + iBodyLen) {
			return; // Message incomplete. Waiting for more bytes.
		}
		evbuffer_drain(input, iHeadLen);

		char* pBuf = (char*)malloc(iBodyLen + 1); // Add space for trailing '\0'.

		evbuffer_remove(input, pBuf, iBodyLen);
		pBuf[iBodyLen] = '\0';

		std::string requestStr(pBuf, iBodyLen);
		free(pBuf);

		if (head.iFlags & PH_COMPRESSED)
		{
			Decompressor::LZ4DecompressorImpl decompressor;
			auto optDate = decompressor.decompress(requestStr);
			if (optDate.has_value())
			{
				this->recv(this->iSerialNum, head.iOpcode, optDate.value());
			}
			else
			{
				//Error
			}
		}
		else
		{
			this->recv(this->iSerialNum, head.iOpcode, requestStr);
		}


		size_t iCurLen = evbuffer_get_length(input);
		if (iCurLen < iHeadLen)
		{
			return;
		}

		//pBuf:申请的内存，在逻辑线程释放 free(pBuf))
	}
}

void APie::ClientConnection::recv(uint64_t iSerialNum, uint32_t iOpcode, std::string& requestStr)
{
	auto optionalData = Api::OpcodeHandlerSingleton::get().client.getType(iOpcode);
	if (!optionalData)
	{
		PBForward *itemObjPtr = new PBForward;
		itemObjPtr->type = ConnetionType::CT_CLIENT;
		itemObjPtr->iSerialNum = this->iSerialNum;
		itemObjPtr->iOpcode = iOpcode;
		itemObjPtr->sMsg = requestStr;

		Command command;
		command.type = Command::pb_forward;
		command.args.pb_forward.ptrData = itemObjPtr;

		auto ptrLogic = APie::CtxSingleton::get().getLogicThread();
		if (ptrLogic == nullptr)
		{
			std::stringstream ss;
			ss << "getLogicThread null|iSerialNum:" << iSerialNum << "|iOpcode:" << iOpcode;
			ASYNC_PIE_LOG("ClientConnection/recv", PIE_CYCLE_HOUR, PIE_ERROR, "%s", ss.str().c_str());
			return;
		}
		ptrLogic->push(command);
		return;
	}

	std::string sType = optionalData.value();
	auto ptrMsg = Api::PBHandler::createMessage(sType);
	if (ptrMsg == nullptr)
	{
		return;
	}

	std::shared_ptr<::google::protobuf::Message> newMsg(ptrMsg);
	bool bResult = newMsg->ParseFromString(requestStr);
	if (!bResult)
	{
		return;
	}

	//newMsg->PrintDebugString();

	PBRequest *itemObjPtr = new PBRequest;
	itemObjPtr->type = ConnetionType::CT_CLIENT;
	itemObjPtr->iSerialNum = this->iSerialNum;
	itemObjPtr->iOpcode = iOpcode;
	itemObjPtr->ptrMsg = newMsg;

	Command command;
	command.type = Command::pb_reqeust;
	command.args.pb_reqeust.ptrData = itemObjPtr;

	auto ptrLogic = APie::CtxSingleton::get().getLogicThread();
	if (ptrLogic == nullptr)
	{
		return;
	}
	ptrLogic->push(command);
}

void APie::ClientConnection::readcb()
{
	switch (this->iType)
	{
	case ProtocolType::PT_HTTP:
	{
		this->readHttp();
		break;
	}
	case ProtocolType::PT_PB:
	{
		this->readPB();
		break;
	}
	default:
		break;
	}
}

void APie::ClientConnection::writecb()
{

}

void APie::ClientConnection::eventcb(short what)
{
	if (what & BEV_EVENT_EOF)
	{
		std::stringstream ss;
		ss << "passive|" << what << "|BEV_EVENT_EOF" << "|Connection closed.";
		//printf("Connection closed.\n");
		this->close(ss.str());
	} 
	else if (what & BEV_EVENT_ERROR)
	{
		std::stringstream ss;
		ss << "passive|" << what << "|BEV_EVENT_ERROR" << "|Got an error on the connection|" << strerror(errno);
		//printf("Got an error on the connection: %s\n",strerror(errno));/*XXX win32*/
		this->close(ss.str(),BEV_EVENT_ERROR);

	} 
	else if (what & BEV_EVENT_CONNECTED) 
	{
		this->sendConnectResultCmd(0);
	}
	else if (what & BEV_EVENT_TIMEOUT)
	{
		std::stringstream ss;
		ss << "active|" << what << "|BEV_EVENT_TIMEOUT";
		this->close(ss.str());
	}
	else
	{
		std::stringstream ss;
		ss << "passive|" << what << "|OTHER";
		this->close(ss.str());
	}
	
}


void APie::ClientConnection::SetConnectTo(const std::string& sAddress, uint16_t iPort)
{
	this->sListenAddress = sAddress;
	this->iListenPort = iPort;
}

void APie::ClientConnection::handleSend(const char *data, size_t size)
{
	if (NULL != this->bev)
	{
		int rc = bufferevent_write(this->bev, data, size);
		if (rc != 0)
		{
			PIE_LOG("Exception/Exception",PIE_CYCLE_DAY,PIE_ERROR,"ClientConnection|handleSend Error:%d|%*s",rc,size,data);
		}
		else
		{
			//std::stringstream ss;
			//ss << "Session/" << this->iSerialNum;
			//std::string sFile = ss.str();
			//pieLog(sFile.c_str(),PIE_CYCLE_DAY,PIE_DEBUG,"ClientConnection|handleSend:data|%s|size|%d",data,size);
		}

	}
}

void APie::ClientConnection::handleClose()
{
	std::stringstream ss;
	ss << "ClientConnection|active|" << "call By C_closeSocket";
	this->close(ss.str(),0,1);
}

static void client_readcb(struct bufferevent *bev, void *arg)
{
	APie::ClientConnection *ptrSession = (APie::ClientConnection *)arg;
	ptrSession->readcb();
}

static void client_writecb(struct bufferevent *bev, void *arg)
{
	APie::ClientConnection *ptrSession = (APie::ClientConnection *)arg;
	ptrSession->writecb();
}

static void client_eventcb(struct bufferevent *bev, short what, void *arg)
{
	APie::ClientConnection *ptrSession = (APie::ClientConnection *)arg;
	ptrSession->eventcb(what);
}

std::shared_ptr<APie::ClientConnection> APie::ClientConnection::createClient(uint32_t threadId, struct event_base *base, DialParameters* ptrDial)
{
	std::shared_ptr<APie::ClientConnection> ptrSharedClient(nullptr);

	uint64_t iSerialNum = ptrDial->iCurSerialNum;
	uint16_t iPort = ptrDial->iPort;
	ProtocolType iCodecType =  ptrDial->iCodecType;
	const char* ip = ptrDial->sIp.c_str();

	uint32_t iResult = 0;
	struct bufferevent * bev = NULL;

	do
	{
		struct sockaddr_in s;
		memset(&s, 0, sizeof(struct sockaddr_in));
		s.sin_family = AF_INET;
		s.sin_port = htons(iPort);
		//s.sin_addr.s_addr = inet_addr(ip);
		::inet_pton(AF_INET, ip, &s.sin_addr);

		if (s.sin_addr.s_addr == INADDR_NONE)
		{
			APie::Network::getInAddr(&s.sin_addr, ip);
		}

		bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
		if (NULL == bev)
		{
			iResult = 2;
			break;
		}

		if (bufferevent_socket_connect(bev, (struct sockaddr*)&s, sizeof(struct sockaddr)) < 0)
		{
			fprintf(stderr, "bufferevent_socket_connect return <0 ! errno=%d,%s.", errno, strerror(errno));
			bufferevent_free(bev);
			bev = NULL;

			iResult = 3;
			break;
		}

		evutil_socket_t fd = bufferevent_getfd(bev);
		if (-1 == fd)
		{
			fprintf(stderr, "bufferevent_getfd return -1 ! ");

			bufferevent_free(bev);
			bev = NULL;
			iResult = 4;
			break;
		}

		//  Disable Nagle's algorithm.
		int flag = 1;
		int rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
		if (rc != 0)
		{
			PIE_LOG("Exception/Exception", PIE_CYCLE_DAY, PIE_WARNING, "processActiveConnect|setsockopt|TCP_NODELAY:%d|%s:%d", rc, ip, iPort);
		}
		//assert(rc == 0);

		int on = 1;
		rc = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&on, sizeof(on));
		if (rc != 0)
		{
			PIE_LOG("Exception/Exception", PIE_CYCLE_DAY, PIE_WARNING, "processActiveConnect|setsockopt|SO_KEEPALIVE:%d|%s:%d", rc, ip, iPort);
		}
		//assert(rc == 0);

		std::string sAddress(ip);
		ClientConnection *ptrConnectSession = new (std::nothrow) ClientConnection(iSerialNum, bev, sAddress, iPort, iCodecType, threadId);
		if (NULL == ptrConnectSession)
		{
			fprintf(stderr, "New ClientConnection Error!");
			bufferevent_free(bev);
			bev = NULL;
			iResult = 5;
			break;
		}

		ptrSharedClient.reset(ptrConnectSession);
		APie::Event::DispatcherImpl::addClientConnection(ptrSharedClient);


		bufferevent_setcb(bev, client_readcb, client_writecb, client_eventcb, ptrConnectSession);
		bufferevent_enable(bev, EV_READ | EV_WRITE);

		struct timeval tv_read;
		tv_read.tv_sec = 600;
		tv_read.tv_usec = 0;
		struct timeval tv_write;
		tv_write.tv_sec = 600;
		tv_write.tv_usec = 0;
		bufferevent_set_timeouts(bev, &tv_read, &tv_write);
	} while (false);

	if (0 != iResult)
	{
		Command cmd;
		cmd.type = Command::dial_result;
		cmd.args.dial_result.ptrData = new DialResult();
		cmd.args.dial_result.ptrData->iResult = iResult;
		cmd.args.dial_result.ptrData->iSerialNum = iSerialNum;

		APie::CtxSingleton::get().getLogicThread()->push(cmd);
	}


	return ptrSharedClient;
}