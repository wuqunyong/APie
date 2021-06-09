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
#include <functional>
#include <optional>

#include <stdlib.h>

#include <event2/util.h>
#include <google/protobuf/message.h>

#include "../network/server_connection.h"

#include "../http/http_request.h"
#include "../http/http_response.h"

#include "i_poll_events.hpp"

#include "../serialization/protocol_head.h"

namespace APie
{
	struct PassiveConnect
	{
		evutil_socket_t iFd;
		ProtocolType iType;
		std::string sIp;
		std::string sPeerIp;
		uint32_t iMaskFlag = 0;
	};

	struct PBRequest
	{
		ConnetionType type;
		uint64_t iSerialNum;
		uint32_t iOpcode;
		std::shared_ptr<::google::protobuf::Message> ptrMsg;
	};

	struct PBForward
	{
		ConnetionType type;
		uint64_t iSerialNum;
		uint32_t iOpcode;
		std::string sMsg;
	};

	struct SendData
	{
		ConnetionType type;
		uint64_t iSerialNum;
		std::string sData;
	};

	struct SendDataByFlag
	{
		ConnetionType type;
		uint64_t iSerialNum;
		ProtocolHead head;
		std::string sBody;
	};

	struct LogCmd
	{
		std::string sFile;
		int iCycle;
		int iLevel;
		std::string sMsg;
		bool bIgnoreMore = false;
	};

	struct DialParameters
	{
		std::string sIp;
		uint16_t iPort;
		ProtocolType iCodecType;
		uint64_t iCurSerialNum = 0;
		uint32_t iMaskFlag = 0;
	};

	struct DialResult
	{
		uint64_t iSerialNum;
		uint32_t iResult;
		std::string sLocalIp;
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
	
	struct SetServerSessionAttr
	{
		uint64_t iSerialNum;
		std::optional<std::string> optClientRandom;
		std::optional<std::string> optServerRandom;
		std::optional<std::string> optKey;
	};

	struct SetClientSessionAttr
	{
		uint64_t iSerialNum;
		std::optional<std::string> optKey;
	};

	struct LogicCmd
	{
		std::string sCmd;
	};

	struct LogicAsyncCallFunctor
	{
		std::function<void()> callFunctor;
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
			pb_forward,
			send_data,
			send_data_by_flag, // PH_COMPRESSED, PH_CRYPTO
			dial,
			dial_result,
			set_server_session_attr,
			set_client_session_attr,

			async_log,
			metric_data,

			logic_cmd,
			logic_async_call_functor,

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
				PBForward* ptrData;
			} pb_forward;

			struct {
				SendData* ptrData;
			} send_data;
			
			struct {
				SendDataByFlag* ptrData;
			} send_data_by_flag;

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
				SetServerSessionAttr* ptrData;
			} set_server_session_attr;

			struct {
				SetClientSessionAttr* ptrData;
			} set_client_session_attr;

			struct {
				LogicCmd* ptrData;
			} logic_cmd;

			struct {
				LogicAsyncCallFunctor* ptrData;
			} logic_async_call_functor;

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

