#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <tuple>

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

	using RpcServerCb = std::function<std::tuple<uint32_t, std::string>(const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args)>;

	class RpcServer
	{
	public:
		bool init();

		bool registerOpcodes(::rpc_msg::RPC_OPCODES opcodes, RpcServerCb cb);
		bool asyncReply(uint64_t iSerialNum, const rpc_msg::CLIENT_IDENTIFIER& client, uint32_t errCode, const std::string& replyData);

		static void handleRequest(uint64_t iSerialNum, ::rpc_msg::RPC_REQUEST request);

	private:
		RpcServerCb find(::rpc_msg::RPC_OPCODES opcodes);

	private:
		std::map<::rpc_msg::RPC_OPCODES, RpcServerCb> m_register;
	};

	typedef APie::ThreadSafeSingleton<RpcServer> RpcServerSingleton;
} // namespace Network
} // namespace APie
