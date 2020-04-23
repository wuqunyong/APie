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

#include "../network/i_poll_events.hpp"

#include <event2/bufferevent.h>

namespace Envoy
{
	enum class ProtocolType
	{
		PT_HTTP = 0,
		PT_PB,
	};

    class Connection :
        public i_poll_events,
		public std::enable_shared_from_this<Connection>
    {
    public:
		Connection(uint64_t iSerialNum, bufferevent *bev, ProtocolType iType);

		uint64_t getSerialNum();

		void readcb();
		void writecb();
		void eventcb(short what);


		void close(std::string sInfo);
        ~Connection();

	public:
		void handleSend(const char *data, size_t size);
		void handleClose();

		static bool validProtocol(ProtocolType iType);
	private:
		void sendConnectionClose();

		void readHttp();
		void readPB();
		void recv(uint64_t iSerialNum, char* pBuf, uint32_t iLen);

    private:
		ProtocolType iType;
		uint64_t iSerialNum;
		bufferevent *bev;

		//HttpRequestDecoder decoder;

		Connection(const Connection&) = delete;
        const Connection &operator = (const Connection&) = delete;
    };

}
