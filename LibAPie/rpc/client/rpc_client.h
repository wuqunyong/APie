#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "../../api/api.h"
#include "../../api/pb_handler.h"

#include "../../network/ctx.h"
#include "../../network/address.h"
#include "../../network/output_stream.h"
#include "../../network/windows_platform.h"
#include "../../singleton/threadsafe_singleton.h"

#include "../init.h"

#include <event2/util.h>


namespace APie {
namespace RPC {

	using RpcReplyCb = std::function<void(const rpc_msg::STATUS& status, const std::string& replyData)>;

	class RpcClient
	{
	public:
		bool init();
		bool callByRoute(::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, ::google::protobuf::Message& args, RpcReplyCb reply = nullptr, ::rpc_msg::CONTROLLER controller = ::rpc_msg::CONTROLLER::default_instance());

		void handleTimeout();

	public:
		static void handleResponse(uint64_t iSerialNum, const ::rpc_msg::RPC_RESPONSE& response);

	private:
		bool call(::rpc_msg::CONTROLLER controller, ::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, ::google::protobuf::Message& args, RpcReplyCb reply = nullptr);

		RpcReplyCb find(uint64_t seqId);
		void del(uint64_t seqId);

	private:
		std::map<uint64_t, RpcReplyCb> m_reply;
		std::map<uint64_t, uint64_t> m_expireAt; //ms
		uint64_t m_iSeqId = 0;
		uint64_t m_iCheckTimeoutAt = 60;

		static uint64_t TIMEOUT_DURATION;
		static uint64_t CHECK_INTERVAL;
	};

	typedef APie::ThreadSafeSingleton<RpcClient> RpcClientSingleton;

} // namespace RPC
} // namespace APie
