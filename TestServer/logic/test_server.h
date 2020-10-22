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

	class TestServerMgr
	{
	public:
		std::tuple<uint32_t, std::string> init();
		std::tuple<uint32_t, std::string> start();
		std::tuple<uint32_t, std::string> ready();
		void exit();

	public:
		static void onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg);

		static void handleResponseClientLogin(uint64_t iSerialNum, const ::login_msg::MSG_RESPONSE_CLIENT_LOGIN& response);

	public:
		std::shared_ptr<ClientProxy> m_ptrClientProxy;
	};


	using TestServerMgrSingleton = ThreadSafeSingleton<TestServerMgr>;
}
