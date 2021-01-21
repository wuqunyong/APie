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

	class LoginMgr
	{
	public:
		std::tuple<uint32_t, std::string> init();
		std::tuple<uint32_t, std::string> start();
		std::tuple<uint32_t, std::string> ready();
		void exit();

	public:
		// CMD
		static void onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg);

		// PubSub
		static void onServerPeerClose(uint64_t topic, ::google::protobuf::Message& msg);

		// CLIENT OPCODE
		static void handleAccountLogin(uint64_t iSerialNum, const ::login_msg::MSG_REQUEST_ACCOUNT_LOGIN_L& request);

	private:

	};

	using LoginMgrSingleton = ThreadSafeSingleton<LoginMgr>;
}
