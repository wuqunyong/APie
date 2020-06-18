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
#include <vector>
#include <stdlib.h>

#include <event2/util.h>
#include <google/protobuf/message.h>

#include "../network/connection.h"

namespace APie
{
	struct PassiveConnect
	{
		evutil_socket_t iFd;
		ProtocolType iType;
	};

	struct PBRequest
	{
		uint64_t iSerialNum;
		uint32_t iOpcode;
		std::shared_ptr<::google::protobuf::Message> ptrMsg;
	};

	struct SendData
	{
		uint64_t iSerialNum;
		std::string sData;
	};

	struct LogCmd
	{
		std::string sFile;
		int iCycle;
		int iLevel;
		std::string sMsg;
	};

    //  This structure defines the commands that can be sent between threads.
    class Command
    {
	public:
		Command()
		{
			type = invalid_type;
		}

		//  Object to process the command.
		//class Object *destination;

		enum type_t
		{
			invalid_type = 0,

			passive_connect,
			pb_reqeust,
			send_data,

			async_log,

			done
        } type;

        union {
			struct {
			} invalid_type;

			struct {
				PassiveConnect* ptrData;
			} passive_connect;

			struct {
				PBRequest* ptrData;
			} pb_reqeust;

			struct {
				SendData* ptrData;
			} send_data;
			
			struct {
				LogCmd* ptrData;
			} async_log;

			struct {
			} done;

        } args;
    };

    //  Function to deallocate dynamically allocated components of the command.
    void deallocateCommand(Command* cmd);

}    

