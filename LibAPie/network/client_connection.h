#pragma once

#include <stdint.h>
#include <string>

#include <event2/bufferevent.h>

#include "windows_platform.h"
#include "i_poll_events.hpp"


namespace APie
{

    class ConnectSession :
        public i_poll_events
    {
    public:
		typedef uint64_t integer_t;
        ConnectSession(integer_t iSerialNum, bufferevent *bev, std::string address, short port, uint32_t threadId);

		uint64_t getSerialNum();

		void readcb();
		void writecb();
		void eventcb(short what);

		void SetConnectTo(const std::string& sAddress, uint16_t iPort);

		void close(std::string sInfo, int iCode=0, int iActive=0);
        ~ConnectSession();

	public:
		void handleSend(const char *data, size_t size);
		void handleClose();

	private:
		void sendCloseCmd(uint32_t iResult, const std::string& sInfo, uint32_t iActive);
		void sendConnectResultCmd(uint32_t iResult);

    private:
		integer_t iSerialNum;
		bufferevent *bev;

		std::string address;
		uint16_t port;

		std::string sListenAddress;
		uint16_t iListenPort;

		std::string sLocalAddress;
		uint16_t iLocalPort;

		uint32_t iThreadId;

		//class IOThread* ptrThreadObj;

		//HttpResponseDecoder decoder;

        ConnectSession(const ConnectSession&);
        const ConnectSession &operator = (const ConnectSession&);
    };

}

