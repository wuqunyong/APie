#include "rpc_client.h"

#include <chrono>

#include "../../network/logger.h"
#include "../../event/nats_proxy.h"

#include "../../pb_msg.h"

namespace APie {
namespace RPC {

	uint64_t RpcClient::TIMEOUT_DURATION(60*1000);
	uint64_t RpcClient::CHECK_INTERVAL(100);

	bool RpcClient::init()
	{
		uint64_t curTime = CtxSingleton::get().getCurMilliseconds();
		m_iSeqId = 0;
		m_iCheckTimeoutAt = curTime + CHECK_INTERVAL;

		APie::Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OPCODE_ID::OP_RPC_RESPONSE, RpcClient::handleResponse);
		APie::Api::OpcodeHandlerSingleton::get().server.bind(::opcodes::OPCODE_ID::OP_RPC_RESPONSE, RpcClient::handleResponse);

		return true;
	}

	bool RpcClient::callByRoute(::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, ::google::protobuf::Message& args, 
		RpcReplyCb reply, std::optional<::rpc_msg::CONTROLLER> controllerOpt)
	{
		::rpc_msg::CONTROLLER controller;
		if (controllerOpt.has_value())
		{
			controller = controllerOpt.value();
		}

#ifdef USE_NATS_PROXY
		rpc_msg::STATUS status;

		bool bResult = APie::Event::NatsSingleton::get().isConnect(APie::Event::NatsManager::E_NT_Realm);
		if (!bResult)
		{
			ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "nats not connect|server:%s|opcodes:%d|args:%s",
				server.ShortDebugString().c_str(), opcodes, args.ShortDebugString().c_str());

			status.set_code(opcodes::SC_Rpc_RouteEmpty);
			if (reply)
			{
				reply(status, "");
			}
			return false;
		}
#else
		rpc_msg::STATUS status;
		auto routeList = EndPointMgrSingleton::get().getEndpointsByType(::common::EPT_Route_Proxy);
		if (routeList.empty())
		{
			ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "route list empty|server:%s|opcodes:%d|args:%s",
				server.ShortDebugString().c_str(), opcodes, args.ShortDebugString().c_str());

			status.set_code(opcodes::SC_Rpc_RouteEmpty);
			if (reply)
			{
				reply(status, "");
			}
			return false;
		}

		auto establishedList = EndPointMgrSingleton::get().getEstablishedEndpointsByType(::common::EPT_Route_Proxy);
		if (establishedList.empty())
		{
			ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "route established empty|server:%s|opcodes:%d|args:%s",
				server.ShortDebugString().c_str(), opcodes, args.ShortDebugString().c_str());

			status.set_code(opcodes::SC_Rpc_RouteEstablishedEmpty);
			if (reply)
			{
				reply(status, "");
			}
			return false;
		}

		EndPoint target;
		target.type = server.type();
		target.id = server.id();
		uint32_t iHash = CtxSingleton::get().generateHash(target);

		uint32_t index = iHash % establishedList.size();
		EndPoint route = establishedList[index];

		auto serialOpt = EndPointMgrSingleton::get().getSerialNum(route);
		if (!serialOpt.has_value())
		{
			ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "route closed|server:%s|opcodes:%d|args:%s",
				server.ShortDebugString().c_str(), opcodes, args.ShortDebugString().c_str());

			status.set_code(opcodes::SC_Rpc_RouteSerialNumInvalid);
			if (reply)
			{
				reply(status, "");
			}
			return false;
		}

		controller.set_serial_num(serialOpt.value());
#endif
		return this->call(controller, server, opcodes, args, reply);
	}

	bool RpcClient::callByRouteWithServerStream(::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, ::google::protobuf::Message& args, 
		RpcReplyCb reply, std::optional<::rpc_msg::CONTROLLER> controllerOpt)
	{
		::rpc_msg::CONTROLLER controller;
		if (controllerOpt.has_value())
		{
			controller = controllerOpt.value();
		}

		controller.set_server_stream(true);

		std::optional<::rpc_msg::CONTROLLER> opt = std::make_optional(controller);
		return this->callByRoute(server, opcodes, args, reply, opt);
	}

	bool RpcClient::multiCallByRoute(std::vector<std::tuple<::rpc_msg::CHANNEL, ::rpc_msg::RPC_OPCODES, std::string>> methods, 
		RpcMultiReplyCb reply, std::optional<::rpc_msg::CONTROLLER> controllerOpt)
	{
		::rpc_msg::CONTROLLER controller;
		if (controllerOpt.has_value())
		{
			controller = controllerOpt.value();
		}

		std::vector<std::tuple<rpc_msg::STATUS, std::string>> replyData;

		rpc_msg::STATUS status;
		if (methods.empty())
		{
			ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "methods empty");
			
			status.set_code(opcodes::SC_Rpc_InvalidArgs_MethodsEmpty);
			reply(status, replyData);

			return false;
		}


#ifdef USE_NATS_PROXY
		bool bResult = APie::Event::NatsSingleton::get().isConnect(APie::Event::NatsManager::E_NT_Realm);
		if (!bResult)
		{
			status.set_code(opcodes::SC_Rpc_RouteEmpty);
			if (reply)
			{
				while (replyData.size() < methods.size())
				{
					rpc_msg::STATUS responseStatus;
					responseStatus.set_code(opcodes::SC_Rpc_NotSend);

					std::tuple<rpc_msg::STATUS, std::string> response(responseStatus, "");
					replyData.push_back(response);
				}
				reply(status, replyData);
			}
			return false;
		}
#else
		auto routeList = EndPointMgrSingleton::get().getEndpointsByType(::common::EPT_Route_Proxy);
		if (routeList.empty())
		{
			ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "route list empty");

			status.set_code(opcodes::SC_Rpc_RouteEmpty);
			if (reply)
			{
				while (replyData.size() < methods.size())
				{
					rpc_msg::STATUS responseStatus;
					responseStatus.set_code(opcodes::SC_Rpc_NotSend);

					std::tuple<rpc_msg::STATUS, std::string> response(responseStatus, "");
					replyData.push_back(response);
				}
				reply(status, replyData);
			}
			return false;
		}

		auto establishedList = EndPointMgrSingleton::get().getEstablishedEndpointsByType(::common::EPT_Route_Proxy);
		if (establishedList.empty())
		{
			ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "route established empty");

			status.set_code(opcodes::SC_Rpc_RouteEstablishedEmpty);
			if (reply)
			{
				while (replyData.size() < methods.size())
				{
					rpc_msg::STATUS responseStatus;
					responseStatus.set_code(opcodes::SC_Rpc_NotSend);

					std::tuple<rpc_msg::STATUS, std::string> response(responseStatus, "");
					replyData.push_back(response);
				}
				reply(status, replyData);
			}
			return false;
		}

		// check
		for (const auto& method : methods)
		{
			EndPoint target;
			target.type = std::get<0>(method).type();
			target.id = std::get<0>(method).id();
			uint32_t iHash = CtxSingleton::get().generateHash(target);

			uint32_t index = iHash % establishedList.size();
			EndPoint route = establishedList[index];

			auto serialOpt = EndPointMgrSingleton::get().getSerialNum(route);
			if (!serialOpt.has_value())
			{
				ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "route closed");

				status.set_code(opcodes::SC_Rpc_RouteSerialNumInvalid);
				if (reply)
				{
					while (replyData.size() < methods.size())
					{
						rpc_msg::STATUS responseStatus;
						responseStatus.set_code(opcodes::SC_Rpc_NotSend);

						std::tuple<rpc_msg::STATUS, std::string> response(responseStatus, "");
						replyData.push_back(response);
					}
					reply(status, replyData);
				}

				return false;
			}
		}
#endif

		auto sharedPtrPending = std::make_shared<MultiCallPending>();
		sharedPtrPending->iCount = methods.size();
		sharedPtrPending->iCompleted = 0;

		for (const auto& method : methods)
		{
			EndPoint target;
			target.type = std::get<0>(method).type();
			target.id = std::get<0>(method).id();
			uint32_t iHash = CtxSingleton::get().generateHash(target);

			uint64_t iSerialNum = 0;

#ifdef USE_NATS_PROXY
		// Nothing
#else
			uint32_t index = iHash % establishedList.size();
			EndPoint route = establishedList[index];

			auto serialOpt = EndPointMgrSingleton::get().getSerialNum(route);
			if (!serialOpt.has_value())
			{
				ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "route closed, set serialNum:0");

				// ����ʧ��
				serialOpt = std::make_optional(0);
			}

			iSerialNum = serialOpt.value();
#endif

			m_iSeqId++;

			uint64_t iCurSeqId = m_iSeqId;
			controller.set_serial_num(iSerialNum);
			controller.set_seq_id(iCurSeqId);

			sharedPtrPending->seqIdVec.push_back(iCurSeqId);

			auto pendingCb = [sharedPtrPending, iCurSeqId, reply](const rpc_msg::STATUS& status, const std::string& replyData) mutable {
				sharedPtrPending->iCompleted = sharedPtrPending->iCompleted + 1;
				sharedPtrPending->replyData[iCurSeqId] = std::make_tuple(status, replyData);

				if (sharedPtrPending->iCompleted >= sharedPtrPending->iCount)
				{
					bool bHasError = false;
					for (const auto& seqId : sharedPtrPending->seqIdVec)
					{
						auto findIte = sharedPtrPending->replyData.find(seqId);
						if (findIte != sharedPtrPending->replyData.end())
						{
							if (std::get<0>(findIte->second).code() != opcodes::SC_Ok)
							{
								bHasError = true;
							}

							sharedPtrPending->result.push_back(findIte->second);
						}
						else
						{
							rpc_msg::STATUS responseStatus;
							responseStatus.set_code(opcodes::SC_Rpc_NotReceivedReply);

							std::tuple<rpc_msg::STATUS, std::string> response(responseStatus, "");
							sharedPtrPending->result.push_back(response);
							bHasError = true;
						}
					}

					rpc_msg::STATUS resultStatus;
					resultStatus.set_code(opcodes::SC_Ok);
					if (bHasError)
					{
						resultStatus.set_code(opcodes::SC_Rpc_Partial_Error);
					}

					if (reply)
					{
						sharedPtrPending->iCallTimes++;
						reply(resultStatus, sharedPtrPending->result);
					}
				}

			};
			this->callWithStrArgs(controller, std::get<0>(method), std::get<1>(method), std::get<2>(method), pendingCb);
		}

		return true;
	}


	bool RpcClient::call(::rpc_msg::CONTROLLER controller, ::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, ::google::protobuf::Message& args, RpcReplyCb reply)
	{
		std::string strArgs = args.SerializeAsString();

		bool bResult = this->callWithStrArgs(controller, server, opcodes, strArgs, reply);
		if (!bResult)
		{
			ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "send error|server:%s|opcodes:%d|args:%s",
				server.ShortDebugString().c_str(), opcodes, args.ShortDebugString().c_str());
		}

		return bResult;
	}

	bool RpcClient::callWithStrArgs(::rpc_msg::CONTROLLER controller, ::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, const std::string& args, RpcReplyCb reply)
	{
		rpc_msg::STATUS status;

		uint64_t iCurSeqId = 0;
		if (controller.seq_id() == 0)
		{
			m_iSeqId++;
			iCurSeqId = m_iSeqId;
		}
		else
		{
			iCurSeqId = controller.seq_id();
		}


		uint64_t curTime = CtxSingleton::get().getCurMilliseconds();

		uint64_t iExpireAt = curTime + TIMEOUT_DURATION;
		if (controller.timeout_ms() != 0)
		{
			iExpireAt = curTime + controller.timeout_ms();
		}

		::rpc_msg::CHANNEL client;
		client.set_type(APie::CtxSingleton::get().identify().type);
		client.set_id(APie::CtxSingleton::get().identify().id);
		client.set_realm(APie::CtxSingleton::get().identify().realm);


		::rpc_msg::RPC_REQUEST request;
		*request.mutable_client()->mutable_stub() = client;
		request.mutable_client()->set_seq_id(iCurSeqId);
		request.mutable_client()->set_required_reply(false);

		if (server.realm() == 0)
		{
			server.set_realm(APie::CtxSingleton::get().identify().realm);
		}

		*request.mutable_server()->mutable_stub() = server;
		request.set_opcodes(opcodes);
		request.set_args_data(args);

		if (reply)
		{
			request.mutable_client()->set_required_reply(true);
			m_reply[iCurSeqId] = reply;

			timer_info info = { iCurSeqId, iExpireAt };
			m_expireAt.insert(std::make_pair(iExpireAt, info));

			if (controller.server_stream())
			{
				m_serverStream[iCurSeqId] = true;
			}
		}

		bool bResult = false;

#ifdef USE_NATS_PROXY
		std::string channel = APie::Event::NatsManager::GetTopicChannel(request.server().stub().realm(), request.server().stub().type(), request.server().stub().id());

		::nats_msg::NATS_MSG_PRXOY nats_msg;
		(*nats_msg.mutable_rpc_request()) = request;
		int32_t iRC = APie::Event::NatsSingleton::get().publishNatsMsg(APie::Event::NatsManager::E_NT_Realm, channel, nats_msg);
		if (iRC == 0)
		{
			bResult = true;
		}
#else
		bResult = APie::Network::OutputStream::sendMsg(controller.serial_num(), ::opcodes::OPCODE_ID::OP_RPC_REQUEST, request);
		if (!bResult)
		{
			ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "send error|server:%s|opcodes:%d", server.ShortDebugString().c_str(), opcodes);

			status.set_code(opcodes::SC_Rpc_RouteSendError);
			if (reply)
			{
				reply(status, "");
			}
		}
#endif

		return bResult;
	}

	void RpcClient::handleTimeout()
	{
		uint64_t curTime = CtxSingleton::get().getCurMilliseconds();

		if (curTime > m_iCheckTimeoutAt)
		{
			m_iCheckTimeoutAt = curTime + CHECK_INTERVAL;


			auto it = m_expireAt.begin();
			while (it != m_expireAt.end()) {

				//  If we have to wait to execute the item, same will be true about
				//  all the following items (multimap is sorted). Thus we can stop
				//  checking the subsequent timers and return the time to wait for
				//  the next timer (at least 1ms).
				if (it->first > curTime)
					return;

				//  Trigger the timer.
				auto findIte = m_reply.find(it->second.id);
				if (findIte != m_reply.end())
				{
					::rpc_msg::STATUS status;
					status.set_code(::rpc_msg::RPC_CODE::CODE_Timeout);
					findIte->second(status, "");

					m_reply.erase(findIte);
				}
				m_serverStream.erase(it->second.id);

				//  Remove it from the list of active timers.
				auto o = it;
				++it;
				m_expireAt.erase(o);
			}
		}
	}


	RpcReplyCb RpcClient::find(uint64_t seqId)
	{
		auto findIte = m_reply.find(seqId);
		if (findIte == m_reply.end())
		{
			return nullptr;
		}

		return findIte->second;
	}

	void RpcClient::del(uint64_t seqId)
	{
		auto findIte = m_reply.find(seqId);
		if (findIte != m_reply.end())
		{
			m_reply.erase(findIte);
		}
		m_serverStream.erase(seqId);
	}

	bool RpcClient::isServerStream(uint64_t seqId)
	{
		auto findIte = m_serverStream.find(seqId);
		if (findIte == m_serverStream.end())
		{
			return false;
		}

		return true;
	}

	void RpcClient::handleResponse(uint64_t iSerialNum, const ::rpc_msg::RPC_RESPONSE& response)
	{
		::rpc_msg::CHANNEL server;
		server.set_realm(APie::CtxSingleton::get().identify().realm);
		server.set_type(APie::CtxSingleton::get().identify().type);
		server.set_id(APie::CtxSingleton::get().identify().id);

		if (response.client().stub().type() != server.type() || response.client().stub().id() != server.id())
		{
			if (server.type() != common::EPT_Route_Proxy)
			{
				// TODO
				return;
			}

			if (response.client().router().type() != common::EPT_Route_Proxy)
			{
				// TODO
				return;
			}
			else
			{
				EndPoint sendTarget;
				sendTarget.realm = response.client().stub().realm();
				sendTarget.type = response.client().stub().type();
				sendTarget.id = response.client().stub().id();
				auto serialOpt = EndPointMgrSingleton::get().getSerialNum(sendTarget);
				if (!serialOpt.has_value())
				{
					// TODO
					return;
				}


				bool bResult = APie::Network::OutputStream::sendMsg(serialOpt.value(), ::opcodes::OPCODE_ID::OP_RPC_RESPONSE, response);
				if (!bResult)
				{
					// TODO
				}
				return;
			}
		}

		uint64_t seqId = response.client().seq_id();
		auto replyCb = RpcClientSingleton::get().find(seqId);
		if (replyCb == nullptr)
		{
			//TODO
			return;
		}

		bool hasMore = false;
		auto status = response.status();

		bool isStream = RpcClientSingleton::get().isServerStream(seqId);
		if (isStream)
		{
			hasMore = response.has_more();

			status.set_has_more(hasMore);
			status.set_offset(response.offset());
		}

		replyCb(status, response.result_data());

		if (hasMore)
		{
			//Nothing
		} 
		else
		{
			RpcClientSingleton::get().del(seqId);
		}
		RpcClientSingleton::get().handleTimeout();
	}

} // namespace RPC
} // namespace APie
