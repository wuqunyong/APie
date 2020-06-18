#include "client_connection.h"

#include <sstream>
#include <iostream>

#include "Ctx.h"

#include "../Serialization/ProtocolHead.h"
#include "logger.h"

static const unsigned int MAX_MESSAGE_LENGTH = 16*1024*1024;
static const unsigned int HTTP_BUF_LEN = 8192;

APie::ConnectSession::ConnectSession(integer_t iSerialNum, bufferevent *bev, std::string address, short port, uint32_t threadId)
{
	this->iSerialNum = iSerialNum;
	this->bev = bev;
	this->address = address;
	this->port = port;
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
}

uint64_t APie::ConnectSession::getSerialNum()
{
	return this->iSerialNum;
}

void APie::ConnectSession::close(std::string sInfo, int iCode, int iActive)
{
	std::stringstream ss;
	ss << "close:" << this->sLocalAddress << ":" << this->iLocalPort << "->" 
		<< this->sListenAddress << ":" << this->iListenPort << "|reason:" << sInfo;
	ASYNC_PIE_LOG("ConnectSession/close", PIE_CYCLE_HOUR, PIE_NOTICE, ss.str().c_str());

	//printf("close:%s:%d->%s:%d   reason:%s\n",this->sLocalAddress.c_str(),this->iLocalPort,this->sListenAddress.c_str(),this->iListenPort,sInfo.c_str());

	integer_t iSerialNum = this->iSerialNum;
	//IOThread::unregisterConnectSession(iSerialNum);
	this->sendCloseCmd(iCode, sInfo, iActive);

	//todo

	delete this;
}

void APie::ConnectSession::sendCloseCmd(uint32_t iResult, const std::string& sInfo, uint32_t iActive)
{
	//Command cmd;
	//cmd.type = Command::peer_close;
	//cmd.args.peer_close.ptrData = new PeerClose();
	//cmd.args.peer_close.ptrData->iResult = iResult;
	//cmd.args.peer_close.ptrData->iSerialNum = this->iSerialNum;
	//cmd.args.peer_close.ptrData->sInfo = sInfo;
	//cmd.args.peer_close.ptrData->iActive = iActive;
	//this->getIOThread()->getCtx()->getLogicThread()->push(cmd);
}

void APie::ConnectSession::sendConnectResultCmd(uint32_t iResult)
{
	//Command cmd;
	//cmd.type = Command::active_connect_result;
	//cmd.args.active_connect_result.ptrData = new ActiveConnectResult();
	//cmd.args.active_connect_result.ptrData->iResult = iResult;
	//cmd.args.active_connect_result.ptrData->iSerialNum = this->iSerialNum;
	//this->getIOThread()->getCtx()->getLogicThread()->push(cmd);
}

APie::ConnectSession::~ConnectSession()
{
	if (this->bev != NULL)
	{
		bufferevent_free(this->bev);
		this->bev = NULL;
	}
}

//void APie::ConnectSession::readcb()
//{
//	while (true)
//	{
//		struct evbuffer *input = bufferevent_get_input(this->bev);
//		size_t len = evbuffer_get_length(input);
//
//		ProtocolHead head;
//		ev_ssize_t iHeadLen = sizeof(ProtocolHead);
//		if (len < iHeadLen)
//		{
//			return;
//		}
//
//		ev_ssize_t iRecvLen = evbuffer_copyout(input, &head, iHeadLen);
//		if (iRecvLen < iHeadLen) 
//		{
//			return; // Message incomplete. Waiting for more bytes.
//		}
//
//		uint32_t iBodyLen = head.iBodyLen;
//		if (iBodyLen > MAX_MESSAGE_LENGTH)
//		{
//			// Avoid illegal data (too large message) crashing this client.
//			std::stringstream ss;
//			ss << "active|" << "Message too large: " << iBodyLen << "|Connection closed.";
//			this->close(ss.str());
//			return;
//		}
//
//		if (evbuffer_get_length(input) < iHeadLen + iBodyLen) 
//		{
//			return; // Message incomplete. Waiting for more bytes.
//		}
//		evbuffer_drain(input, iHeadLen);
//
//		char* pBuf = (char*)malloc(iBodyLen + 1); // Add space for trailing '\0'.
//
//		evbuffer_remove(input, pBuf, iBodyLen);
//		pBuf[iBodyLen] = '\0';
//
//		this->recv(this->iSerialNum,pBuf,iBodyLen);
//
//		int iCurLen = evbuffer_get_length(input);
//		if (iCurLen < iHeadLen)
//		{
//			return;
//		}
//		//pBuf:申请的内存，在逻辑线程释放 free(pBuf))
//	}
//}

void APie::ConnectSession::readcb()
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

		//int result = decoder.execute(buf, minLen);
		//if (result != 0)
		//{
		//	return;
		//}
	}
}

void APie::ConnectSession::writecb()
{

}

void APie::ConnectSession::eventcb(short what)
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


void APie::ConnectSession::SetConnectTo(const std::string& sAddress, uint16_t iPort)
{
	this->sListenAddress = sAddress;
	this->iListenPort = iPort;
}

void APie::ConnectSession::handleSend(const char *data, size_t size)
{
	if (NULL != this->bev)
	{
		int rc = bufferevent_write(this->bev, data, size);
		if (rc != 0)
		{
			PIE_LOG("Exception/Exception",PIE_CYCLE_DAY,PIE_ERROR,"ConnectSession|handleSend Error:%d|%*s",rc,size,data);
		}
		else
		{
			//std::stringstream ss;
			//ss << "Session/" << this->iSerialNum;
			//std::string sFile = ss.str();
			//pieLog(sFile.c_str(),PIE_CYCLE_DAY,PIE_DEBUG,"ConnectSession|handleSend:data|%s|size|%d",data,size);
		}

	}
}

void APie::ConnectSession::handleClose()
{
	std::stringstream ss;
	ss << "ConnectSession|active|" << "call By C_closeSocket";
	this->close(ss.str(),0,1);
}
