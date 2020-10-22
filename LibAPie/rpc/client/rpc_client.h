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
	using RpcMultiReplyCb = std::function<void(const rpc_msg::STATUS& status, std::vector<std::tuple<rpc_msg::STATUS, std::string>> replyData)>;

	struct MultiCallPending
	{
		uint32_t iCount = 0;
		uint32_t iCompleted = 0;
		uint32_t iCallTimes = 0;
		std::vector<uint64_t> seqIdVec;
		std::map<uint64_t, std::tuple<rpc_msg::STATUS, std::string>> replyData;
		std::vector<std::tuple<rpc_msg::STATUS, std::string>> result;
	};

	class RpcClient
	{
	public:
		bool init();

		bool callByRoute(::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, ::google::protobuf::Message& args, RpcReplyCb reply = nullptr, ::rpc_msg::CONTROLLER controller = ::rpc_msg::CONTROLLER::default_instance());
		bool callByRouteWithServerStream(::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, ::google::protobuf::Message& args, RpcReplyCb reply = nullptr, ::rpc_msg::CONTROLLER controller = ::rpc_msg::CONTROLLER::default_instance());
		bool multiCallByRoute(std::vector<std::tuple<::rpc_msg::CHANNEL, ::rpc_msg::RPC_OPCODES, std::string>> methods, RpcMultiReplyCb reply = nullptr, ::rpc_msg::CONTROLLER controller = ::rpc_msg::CONTROLLER::default_instance());

		void handleTimeout();

	public:
		static void handleResponse(uint64_t iSerialNum, const ::rpc_msg::RPC_RESPONSE& response);

	private:
		bool call(::rpc_msg::CONTROLLER controller, ::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, ::google::protobuf::Message& args, RpcReplyCb reply = nullptr);
		bool callWithStrArgs(::rpc_msg::CONTROLLER controller, ::rpc_msg::CHANNEL server, ::rpc_msg::RPC_OPCODES opcodes, std::string args, RpcReplyCb reply = nullptr);

		RpcReplyCb find(uint64_t seqId);
		void del(uint64_t seqId);

		bool isServerStream(uint64_t seqId);

	private:
		std::map<uint64_t, RpcReplyCb> m_reply;

		struct timer_info
		{
			uint64_t id;
			uint64_t expireAt;
		};
		std::multimap<uint64_t, timer_info> m_expireAt;
		std::map<uint64_t, bool> m_serverStream;

		uint64_t m_iSeqId = 0;
		uint64_t m_iCheckTimeoutAt = 60;

		static uint64_t TIMEOUT_DURATION;
		static uint64_t CHECK_INTERVAL;

		friend class APie::Network::OutputStream;
	};

	typedef APie::ThreadSafeSingleton<RpcClient> RpcClientSingleton;

} // namespace RPC
} // namespace APie
