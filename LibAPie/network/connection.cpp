#include "../network/connection.h"

#include <sstream>
#include <iostream>

#include <event2/buffer.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../serialization/ProtocolHead.h"
#include "../event/dispatcher_impl.h"


#include "../../PBMsg/login_msg.pb.h"


static const unsigned int MAX_MESSAGE_LENGTH = 16*1024*1024;
static const unsigned int HTTP_BUF_LEN = 8192;

namespace Envoy {

Connection::Connection(uint64_t iSerialNum, bufferevent *bev, ProtocolType iType)
{
	this->iSerialNum = iSerialNum;
	this->bev = bev;
	this->iType = iType;
}

uint64_t Connection::getSerialNum()
{
	return iSerialNum;
}

bool Connection::validProtocol(ProtocolType iType)
{
	bool valid = false;

	switch (iType)
	{
	case ProtocolType::PT_HTTP:
	case ProtocolType::PT_PB:
	{
		valid = true;
		break;
	}
	default:
		break;
	}

	return valid;
}

void Connection::close(std::string sInfo)
{
	this->sendConnectionClose();

	Event::DispatcherImpl::delConnection(this->iSerialNum);
}

Connection::~Connection()
{
	if (this->bev != NULL)
	{
		bufferevent_free(this->bev);
		this->bev = NULL;
	}
}

void Connection::readHttp()
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

void Connection::readPB()
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

		this->recv(this->iSerialNum, pBuf, iBodyLen);


		size_t iCurLen = evbuffer_get_length(input);
		if (iCurLen < iHeadLen)
		{
			return;
		}

		//pBuf:申请的内存，在逻辑线程释放 free(pBuf))
	}
}

void Connection::recv(uint64_t iSerialNum, char* pBuf, uint32_t iLen)
{
	std::string requestStr(pBuf, iLen);
	::login_msg::MSG_CLIENT_LOGINTOL request;
	bool bResult = request.ParseFromString(requestStr);
	if (!bResult)
	{
		return;
	}

	request.PrintDebugString();
}

void Connection::readcb()
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

void Connection::writecb()
{

}

void Connection::eventcb(short what)
{
	if (what & BEV_EVENT_EOF)
	{
		std::stringstream ss;
		ss << "passive|" << what << "|BEV_EVENT_EOF" << "|Connection closed.";
		this->close(ss.str());
	}
	else if (what & BEV_EVENT_ERROR)
	{
		std::stringstream ss;
		ss << "passive|" << what << "|BEV_EVENT_ERROR" << "|Got an error on the connection|" << strerror(errno);
		this->close(ss.str());
	}
	else if (what & BEV_EVENT_CONNECTED)
	{
	}
	else if (what & BEV_EVENT_TIMEOUT)
	{
		std::stringstream ss;
		ss << "active|" << what << "|BEV_EVENT_TIMEOUT";

		if (what & BEV_EVENT_READING)
		{
			ss << "|BEV_EVENT_READING";
		}

		if (what & BEV_EVENT_WRITING)
		{
			ss << "|BEV_EVENT_WRITING";
		}

		this->close(ss.str());
	}
	else
	{
		std::stringstream ss;
		ss << "passive|" << what << "|OTHER";
		this->close(ss.str());
	}

}


void Connection::handleSend(const char *data, size_t size)
{
	if (NULL != this->bev)
	{
		int rc = bufferevent_write(this->bev, data, size);
		if (rc != 0)
		{
			//PIE_LOG("Exception/Exception", PIE_CYCLE_DAY, PIE_ERROR, "Session|handleSend Error:%d|%*s", rc, size, data);
		}
	}
}

void Connection::sendConnectionClose()
{

}

void Connection::handleClose()
{
	std::stringstream ss;
	ss << "Session|active|" << "call By C_closeSocket";
	this->close(ss.str());
}

}