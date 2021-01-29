#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

#include "apie.h"
#include "../../PBMsg/BusinessMsg/login_msg.pb.h"

namespace APie {

	class SceneMgr
	{
	public:
		std::tuple<uint32_t, std::string> init();
		std::tuple<uint32_t, std::string> start();
		std::tuple<uint32_t, std::string> ready();
		void exit();

	public:
		// CMD
		static void onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg);


		// RPC
		static std::tuple<uint32_t, std::string> RPC_handleMultiplexerForward(const ::rpc_msg::CLIENT_IDENTIFIER& client, ::rpc_msg::PRC_Multiplexer_Forward_Args request);


		// CLIENT OPCODE
		static void Forward_handlEcho(::rpc_msg::RoleIdentifier roleIdentifier, ::login_msg::MSG_REQUEST_ECHO request);
	private:

	};

	using SceneMgrSingleton = ThreadSafeSingleton<SceneMgr>;
}
