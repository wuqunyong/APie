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

	bool RpcClient::callByRoute(::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, ::google::protobuf::Message& args, RpcReplyCb reply)
	{
		rpc_msg::STATUS status;
		auto routeList = EndPointMgrSingleton::get().getEndpointsByType(::service_discovery::EPT_Route_Proxy);
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

		auto establishedList = EndPointMgrSingleton::get().getEstablishedEndpointsByType(::service_discovery::EPT_Route_Proxy);
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

		::rpc_msg::CONTROLLER cntl;
		cntl.set_serial_num(serialOpt.value());
		return this->call(cntl, server, opcodes, args, reply);
	}

	bool RpcClient::call(::rpc_msg::CONTROLLER cntl, ::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, ::google::protobuf::Message& args, RpcReplyCb reply)
	{
		rpc_msg::STATUS status;

		m_iSeqId++;

		uint64_t curTime = CtxSingleton::get().getNowMilliseconds();

		uint64_t iExpireAt = curTime + TIMEOUT_DURATION;
		if (cntl.timeout_ms() != 0)
		{
			iExpireAt = curTime + cntl.timeout_ms();
		}

		::rpc_msg::CHANNEL client;
		client.set_type(APie::CtxSingleton::get().identify().type);
		client.set_id(APie::CtxSingleton::get().identify().id);

		::rpc_msg::RPC_REQUEST request;
		*request.mutable_client()->mutable_stub() = client;
		request.mutable_client()->set_seq_id(m_iSeqId);
		request.mutable_client()->set_required_reply(false);

		*request.mutable_server()->mutable_stub() = server;
		request.set_opcodes(opcodes);
		request.set_args_data(args.SerializeAsString());

		if (reply)
		{
			request.mutable_client()->set_required_reply(true);
			m_reply[m_iSeqId] = reply;
			m_expireAt[m_iSeqId] = iExpireAt;
		}

		bool bResult = APie::Network::OutputStream::sendMsg(cntl.serial_num(), ::opcodes::OPCODE_ID::OP_RPC_REQUEST, request);
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

	void RpcClient::handleTimeout()
	{
		uint64_t curTime = CtxSingleton::get().getNowMilliseconds();

		if (curTime > m_iCheckTimeoutAt)
		{
			m_iCheckTimeoutAt = curTime + CHECK_INTERVAL;

			std::vector<uint64_t> delVec;
			for (const auto& item : m_expireAt)
			{
				if (item.second < curTime)
				{
					delVec.push_back(item.first);

					auto findIte = m_reply.find(item.first);
					if (findIte != m_reply.end())
					{
						::rpc_msg::STATUS status;
						status.set_code(::rpc_msg::RPC_CODE::CODE_Timeout);
						findIte->second(status, "");

						m_reply.erase(findIte);
					}
				}
			}

			for (const auto& item : delVec)
			{
				m_expireAt.erase(item);
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
	}

	void RpcClient::handleResponse(uint64_t iSerialNum, const ::rpc_msg::RPC_RESPONSE& response)
	{
		uint64_t seqId = response.client().seq_id();
		auto replyCb = RpcClientSingleton::get().find(seqId);
		if (replyCb == nullptr)
		{
			//TODO
			return;
		}
		replyCb(response.status(), response.result_data());

		RpcClientSingleton::get().del(seqId);
		RpcClientSingleton::get().handleTimeout();
	}

} // namespace RPC
} // namespace APie
