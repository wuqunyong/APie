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

#include "../network/server_connection.h"

#include "../http/http_request.h"
#include "../http/http_response.h"

#include "i_poll_events.hpp"

namespace APie
{
	struct PassiveConnect
	{
		evutil_socket_t iFd;
		ProtocolType iType;
		std::string sIp;
	};

	struct PBRequest
	{
		ConnetionType type;
		uint64_t iSerialNum;
		uint32_t iOpcode;
		std::shared_ptr<::google::protobuf::Message> ptrMsg;
	};

	struct SendData
	{
		ConnetionType type;
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

	struct DialParameters
	{
		std::string sIp;
		uint16_t iPort;
		ProtocolType iCodecType;
		uint64_t iCurSerialNum;
	};

	struct DialResult
	{
		uint64_t iSerialNum;
		uint32_t iResult;
	};

	struct MetricData
	{
		std::string sMetric;
		std::map<std::string, std::string> tag;
		std::map<std::string, double> field;
	};

	struct CloseLocalClient
	{
		uint64_t iSerialNum;
	};

	struct CloseLocalServer
	{
		uint64_t iSerialNum;
	};

	struct ClientPeerClose
	{
		uint64_t iSerialNum;
		uint32_t iResult;
		std::string sInfo;
		uint32_t iActive;
	};

	struct ServerPeerClose
	{
		uint64_t iSerialNum;
		uint32_t iResult;
		std::string sInfo;
		uint32_t iActive;
	};

	struct LogicCmd
	{
		std::string sCmd;
	};

    //  This structure defines the commands that can be sent between threads.
    class Command
    {
	public:
		Command()
		{
			type = invalid_type;
		}

		enum type_t
		{
			invalid_type = 0,

			passive_connect,
			pb_reqeust,
			send_data,
			dial,
			dial_result,

			async_log,
			metric_data,

			logic_cmd,

			close_local_client, //active(LogicThread -> IOThread | ClientProxy::sendClose)
			close_local_server,
			client_peer_close,  //client: passive(IOThread -> LogicThread)
			server_peer_close,  //server: passive(IOThread -> LogicThread)

			recv_http_request,  //server:passive_connect
			send_http_response, //server:passive_connect

			recv_http_response, //client:active_connect


			logic_start,
			logic_exit,
			stop_thread,

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
				MetricData* ptrData;
			} metric_data;

			struct {
				DialParameters* ptrData;
			} dial;

			struct {
				DialResult* ptrData;
			} dial_result;

			struct {
				LogicCmd* ptrData;
			} logic_cmd;

			struct {
				CloseLocalClient* ptrData;
			} close_local_client;

			struct {
				CloseLocalServer* ptrData;
			} close_local_server;

			struct {
				ClientPeerClose* ptrData;
			} client_peer_close;

			struct {
				ServerPeerClose* ptrData;
			} server_peer_close;

			struct {
				uint64_t iSerialNum;
				HttpRequest* ptrData;
			} recv_http_request;

			struct {
				uint64_t iSerialNum;
				HttpResponse* ptrData;
			} send_http_response;

			struct {
				uint64_t iSerialNum;
				HttpResponse* ptrData;
			} recv_http_response;

			struct {
				uint32_t iThreadId;
			} logic_start;

			struct {
				uint32_t iThreadId;
			} logic_exit;

			struct {
				uint32_t iThreadId;
			} stop_thread;

			struct {
			} done;

        } args;
    };

    //  Function to deallocate dynamically allocated components of the command.
    void deallocateCommand(Command* cmd);

}    

