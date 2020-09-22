#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

#include "apie.h"
#include "../../PBMsg/login_msg.pb.h"

namespace APie {

	class SceneMgr
	{
	public:
		void init();
		void start();
		void exit();

	public:
		static std::tuple<uint32_t, std::string> RPC_handleMultiplexerForward(const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args);

		static void Forward_handleLogin(::rpc_msg::RoleIdentifier roleIdentifier, ::login_msg::MSG_REQUEST_CLIENT_LOGIN request);
	private:

	};

	using SceneMgrSingleton = ThreadSafeSingleton<SceneMgr>;
}
