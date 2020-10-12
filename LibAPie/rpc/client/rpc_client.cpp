#include "rpc_client.h"

#include <chrono>

#include "../../network/logger.h"

namespace APie {
namespace RPC {

	uint64_t RpcClient::TIMEOUT_DURATION(60*1000);
	uint64_t RpcClient::CHECK_INTERVAL(100);

	bool RpcClient::init()
	{
		uint64_t curTime = CtxSingleton::get().getNowMilliseconds();
		m_iSeqId = 0;
		m_iCheckTimeoutAt = curTime + CHECK_INTERVAL;

		APie::Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OPCODE_ID::OP_RPC_RESPONSE, RpcClient::handleResponse, ::rpc_msg::RPC_RESPONSE::default_instance());
		APie::Api::OpcodeHandlerSingleton::get().server.bind(::opcodes::OPCODE_ID::OP_RPC_RESPONSE, RpcClient::handleResponse, ::rpc_msg::RPC_RESPONSE::default_instance());

		return true;
	}

	bool RpcClient::callByRoute(::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, ::google::protobuf::Message& args, RpcReplyCb reply, ::rpc_msg::CONTROLLER controller)
	{
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

			status.set_code(opcodes::SC_RPC_RouteSerialNumInvalid);
			if (reply)
			{
				reply(status, "");
			}
			return false;
		}

		controller.set_serial_num(serialOpt.value());
		return this->call(controller, server, opcodes, args, reply);
	}

	bool RpcClient::callByRouteWithServerStream(::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, ::google::protobuf::Message& args, RpcReplyCb reply, ::rpc_msg::CONTROLLER controller)
	{
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

			status.set_code(opcodes::SC_RPC_RouteSerialNumInvalid);
			if (reply)
			{
				reply(status, "");
			}
			return false;
		}

		controller.set_serial_num(serialOpt.value());
		controller.set_server_stream(true);
		return this->call(controller, server, opcodes, args, reply);
	}

	bool RpcClient::multiCallByRoute(std::vector<std::tuple<::rpc_msg::CHANNEL, ::rpc_msg::RPC_OPCODES, std::string>> methods, RpcMultiReplyCb reply, ::rpc_msg::CONTROLLER controller)
	{
		std::vector<std::tuple<rpc_msg::STATUS, std::string>> replyData;

		rpc_msg::STATUS status;
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
					responseStatus.set_code(opcodes::SC_RPC_NotSend);

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
					responseStatus.set_code(opcodes::SC_RPC_NotSend);

					std::tuple<rpc_msg::STATUS, std::string> response(responseStatus, "");
					replyData.push_back(response);
				}
				reply(status, replyData);
			}
			return false;
		}

		auto sharedPtrPending = std::make_shared<MultiCallPending>();
		sharedPtrPending->iCount = methods.size();
		sharedPtrPending->iCompleted = 0;

		uint32_t iIndex = 0;
		for (const auto& methon : methods)
		{
			EndPoint target;
			target.type = std::get<0>(methon).type();
			target.id = std::get<0>(methon).id();
			uint32_t iHash = CtxSingleton::get().generateHash(target);

			uint32_t index = iHash % establishedList.size();
			EndPoint route = establishedList[index];

			auto serialOpt = EndPointMgrSingleton::get().getSerialNum(route);
			if (!serialOpt.has_value())
			{
				ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "route closed");

				status.set_code(opcodes::SC_RPC_RouteSerialNumInvalid);
				if (reply)
				{
					for (const auto& seqId : sharedPtrPending->seqIdVec)
					{
						auto findIte = sharedPtrPending->replyData.find(seqId);
						if (findIte != sharedPtrPending->replyData.end())
						{
							sharedPtrPending->result.push_back(findIte->second);
						}
						else
						{
							rpc_msg::STATUS responseStatus;
							responseStatus.set_code(opcodes::SC_Rpc_Timeout);

							std::tuple<rpc_msg::STATUS, std::string> response(responseStatus, "");
							sharedPtrPending->result.push_back(response);
						}
					}

					while (sharedPtrPending->result.size() < methods.size())
					{
						rpc_msg::STATUS responseStatus;
						responseStatus.set_code(opcodes::SC_RPC_NotSend);

						std::tuple<rpc_msg::STATUS, std::string> response(responseStatus, "");
						sharedPtrPending->result.push_back(response);
					}

					reply(status, sharedPtrPending->result);
				}
				return false;
			}

			m_iSeqId++;

			uint64_t iCurSeqId = m_iSeqId;
			controller.set_serial_num(serialOpt.value());
			controller.set_seq_id(iCurSeqId);

			sharedPtrPending->seqIdVec.push_back(iCurSeqId);

			iIndex++;
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
							sharedPtrPending->result.push_back(findIte->second);
						}
						else
						{
							rpc_msg::STATUS responseStatus;
							responseStatus.set_code(opcodes::SC_Rpc_Timeout);

							std::tuple<rpc_msg::STATUS, std::string> response(responseStatus, "");
							sharedPtrPending->result.push_back(response);
							bHasError = true;
						}
					}

					rpc_msg::STATUS resultStatus;
					resultStatus.set_code(opcodes::SC_Ok);
					if (bHasError)
					{
						resultStatus.set_code(opcodes::SC_Rpc_Timeout);
					}

					if (reply)
					{
						sharedPtrPending->iCallTimes++;
						reply(resultStatus, sharedPtrPending->result);
					}
				}

			};
			this->callWithStrArgs(controller, std::get<0>(methon), std::get<1>(methon), std::get<2>(methon), pendingCb);
		}

		return true;
	}


	bool RpcClient::call(::rpc_msg::CONTROLLER controller, ::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, ::google::protobuf::Message& args, RpcReplyCb reply)
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
		

		uint64_t curTime = CtxSingleton::get().getNowMilliseconds();

		uint64_t iExpireAt = curTime + TIMEOUT_DURATION;
		if (controller.timeout_ms() != 0)
		{
			iExpireAt = curTime + controller.timeout_ms();
		}

		::rpc_msg::CHANNEL client;
		client.set_type(APie::CtxSingleton::get().identify().type);
		client.set_id(APie::CtxSingleton::get().identify().id);

		::rpc_msg::RPC_REQUEST request;
		*request.mutable_client()->mutable_stub() = client;
		request.mutable_client()->set_seq_id(iCurSeqId);
		request.mutable_client()->set_required_reply(false);

		*request.mutable_server()->mutable_stub() = server;
		request.set_opcodes(opcodes);
		request.set_args_data(args.SerializeAsString());

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

		bool bResult = APie::Network::OutputStream::sendMsg(controller.serial_num(), ::opcodes::OPCODE_ID::OP_RPC_REQUEST, request);
		if (!bResult)
		{
			ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "send error|server:%s|opcodes:%d|args:%s", 
				server.ShortDebugString().c_str(), opcodes, args.ShortDebugString().c_str());

			status.set_code(opcodes::SC_RPC_RouteSendError);
			if (reply)
			{
				reply(status, "");
			}
		}

		return bResult;
	}

	bool RpcClient::callWithStrArgs(::rpc_msg::CONTROLLER controller, ::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, std::string args, RpcReplyCb reply)
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


		uint64_t curTime = CtxSingleton::get().getNowMilliseconds();

		uint64_t iExpireAt = curTime + TIMEOUT_DURATION;
		if (controller.timeout_ms() != 0)
		{
			iExpireAt = curTime + controller.timeout_ms();
		}

		::rpc_msg::CHANNEL client;
		client.set_type(APie::CtxSingleton::get().identify().type);
		client.set_id(APie::CtxSingleton::get().identify().id);

		::rpc_msg::RPC_REQUEST request;
		*request.mutable_client()->mutable_stub() = client;
		request.mutable_client()->set_seq_id(iCurSeqId);
		request.mutable_client()->set_required_reply(false);

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

		bool bResult = APie::Network::OutputStream::sendMsg(controller.serial_num(), ::opcodes::OPCODE_ID::OP_RPC_REQUEST, request);
		if (!bResult)
		{
			ASYNC_PIE_LOG("rpc/rpc", PIE_CYCLE_DAY, PIE_ERROR, "send error|server:%s|opcodes:%d", server.ShortDebugString().c_str(), opcodes);

			status.set_code(opcodes::SC_RPC_RouteSendError);
			if (reply)
			{
				reply(status, "");
			}
		}

		return bResult;
	}

	void RpcClient::handleTimeout()
	{
		uint64_t curTime = CtxSingleton::get().getNowMilliseconds();

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
				auto findIte = m_reply.find(it->first);
				if (findIte != m_reply.end())
				{
					::rpc_msg::STATUS status;
					status.set_code(::rpc_msg::RPC_CODE::CODE_Timeout);
					findIte->second(status, "");

					m_reply.erase(findIte);
				}
				m_serverStream.erase(it->first);

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
		replyCb(response.status(), response.result_data());

		bool isStream = RpcClientSingleton::get().isServerStream(seqId);
		if (isStream)
		{
			bool hasMore = response.has_more();
			if (hasMore)
			{
				//Nothing
			}
			else
			{
				RpcClientSingleton::get().del(seqId);
			}
		} 
		else
		{
			RpcClientSingleton::get().del(seqId);
		}
		RpcClientSingleton::get().handleTimeout();
	}

} // namespace RPC
} // namespace APie
