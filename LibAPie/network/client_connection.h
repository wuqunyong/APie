#pragma once

#include <stdint.h>
#include <string>
#include <memory>

#include <event2/bufferevent.h>

#include "windows_platform.h"
#include "i_poll_events.hpp"
#include "Command.h"


namespace APie
{
    class ClientConnection :
        public i_poll_events
    {
    public:
		typedef uint64_t integer_t;
		ClientConnection(integer_t iSerialNum, bufferevent *bev, std::string address, short port, ProtocolType type, uint32_t threadId);

		uint64_t getSerialNum();
		uint32_t getTId();

		void readcb();
		void writecb();
		void eventcb(short what);

		void SetConnectTo(const std::string& sAddress, uint16_t iPort);

		void close(std::string sInfo, int iCode=0, int iActive=0);
        ~ClientConnection();

	public:
		void handleSend(const char *data, size_t size);
		void handleClose();

	private:
		void readHttp();
		void readPB();
		void recv(uint64_t iSerialNum, uint32_t iOpcode, std::string& requestStr);


		void sendCloseCmd(uint32_t iResult, const std::string& sInfo, uint32_t iActive);
		void sendConnectResultCmd(uint32_t iResult);

	public:
		static std::shared_ptr<ClientConnection> createClient(uint32_t threadId, struct event_base *base, DialParameters* ptrDial);

    private:
		integer_t iSerialNum;
		bufferevent *bev;
		ProtocolType iType;

		std::string sListenAddress;
		uint16_t iListenPort;

		std::string sLocalAddress;
		uint16_t iLocalPort;

		uint32_t iThreadId;

		//class IOThread* ptrThreadObj;

		//HttpResponseDecoder decoder;

		ClientConnection(const ClientConnection&);
        const ClientConnection &operator = (const ClientConnection&);
    };

}

