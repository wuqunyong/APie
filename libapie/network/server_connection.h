/*
    Copyright (c) 2007-2011 iMatix Corporation
    Copyright (c) 2007-2011 Other contributors as noted in the AUTHORS file

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <stdint.h>
#include <string>
#include <optional>

#include "../network/i_poll_events.hpp"
#include "../network/object.hpp"

#include "../http/http_request_decoder.h"

#include <event2/bufferevent.h>


namespace APie
{
	struct SetServerSessionAttr;

    class ServerConnection :
        public i_poll_events
    {
    public:
		ServerConnection(uint32_t tid, uint64_t iSerialNum, bufferevent *bev, ProtocolType iType);

		uint64_t getSerialNum();
		uint32_t getTId();

		void readcb();
		void writecb();
		void eventcb(short what);

		void setIp(std::string ip, std::string peerIp);
		void setMaskFlag(uint32_t iFlag);
		uint32_t getMaskFlag();
		std::optional<std::string> getSessionKey();

		void handleSetServerSessionAttr(SetServerSessionAttr* ptrCmd);

		std::string ip();
		std::string peerIp();

		std::string getClientRandom();
		std::string getServerRandom();

		void close(std::string sInfo, uint32_t iCode = 0, uint32_t iActive = 0);
        ~ServerConnection();

	public:
		void handleSend(const char *data, size_t size);
		void handleClose();

		static bool validProtocol(ProtocolType iType);
		static void sendCloseLocalServer(uint64_t iSerialNum);
	private:
		void sendCloseCmd(uint32_t iResult, const std::string& sInfo, uint32_t iActive);

		void readHttp();
		void readPB();
		void recv(uint64_t iSerialNum, uint32_t iOpcode, std::string& requestStr);

    private:
		uint32_t tid_;
		ProtocolType iType;
		uint32_t m_iMaskFlag = 0;
		std::string m_clientRandom;
		std::string m_serverRandom;
		std::optional<std::string>  m_optSessionKey;

		uint64_t iSerialNum;
		bufferevent *bev;

		HttpRequestDecoder decoder;
		std::string sIp;
		std::string sPeerIp;

		ServerConnection(const ServerConnection&) = delete;
        const ServerConnection &operator = (const ServerConnection&) = delete;
    };

}
