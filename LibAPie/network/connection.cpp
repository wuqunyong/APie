#include "../network/connection.h"

#include <sstream>
#include <iostream>

#include <event2/buffer.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../serialization/ProtocolHead.h"
#include "../event/dispatcher_impl.h"
#include "../api/pb_handler.h"
#include "../network/Ctx.h"

#include "../../PBMsg/login_msg.pb.h"




static const unsigned int MAX_MESSAGE_LENGTH = 16*1024*1024;
static const unsigned int HTTP_BUF_LEN = 8192;

namespace APie {

ServerConnection::ServerConnection(uint32_t tid, uint64_t iSerialNum, bufferevent *bev, ProtocolType iType) :
	tid_(tid)
{
	this->iSerialNum = iSerialNum;
	this->bev = bev;
	this->iType = iType;
}

uint64_t ServerConnection::getSerialNum()
{
	return iSerialNum;
}

uint32_t ServerConnection::getTId()
{
	return tid_;
}

bool ServerConnection::validProtocol(ProtocolType iType)
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

void ServerConnection::close(std::string sInfo)
{
	this->sendConnectionClose();

	Event::DispatcherImpl::delConnection(this->iSerialNum);
}

ServerConnection::~ServerConnection()
{
	if (this->bev != NULL)
	{
		bufferevent_free(this->bev);
		this->bev = NULL;
	}
}

void ServerConnection::readHttp()
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

void ServerConnection::readPB()
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

		this->recv(this->iSerialNum, head.iOpcode, requestStr);


		size_t iCurLen = evbuffer_get_length(input);
		if (iCurLen < iHeadLen)
		{
			return;
		}

		//pBuf:申请的内存，在逻辑线程释放 free(pBuf))
	}
}

void ServerConnection::recv(uint64_t iSerialNum, uint32_t iOpcode, std::string& requestStr)
{
	auto optionalData = Api::PBHandlerSingleton::get().getType(iOpcode);
	if (!optionalData)
	{
		return;
	}

	std::string sType = optionalData.value();
	auto ptrMsg = Api::PBHandlerSingleton::get().createMessage(sType);
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

	newMsg->PrintDebugString();

	PBRequest *itemObjPtr = new PBRequest;
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

void ServerConnection::readcb()
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

void ServerConnection::writecb()
{

}

void ServerConnection::eventcb(short what)
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


void ServerConnection::handleSend(const char *data, size_t size)
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

void ServerConnection::sendConnectionClose()
{

}

void ServerConnection::handleClose()
{
	std::stringstream ss;
	ss << "Session|active|" << "call By C_closeSocket";
	this->close(ss.str());
}

}