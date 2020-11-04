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
#include "mock_role.h"


namespace APie {

	class TestServerMgr
	{
	public:
		std::tuple<uint32_t, std::string> init();
		std::tuple<uint32_t, std::string> start();
		std::tuple<uint32_t, std::string> ready();
		void exit();

		void addMockRole(std::shared_ptr<MockRole> ptrMockRole);
		std::shared_ptr<MockRole> findMockRole(uint64_t iRoleId);
		void removeMockRole(uint64_t iRoleId);

	public:
		static void onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg);

		static void handleResponseClientLogin(uint64_t iSerialNum, const ::login_msg::MSG_RESPONSE_CLIENT_LOGIN& response);
		static void handleResponseEcho(uint64_t iSerialNum, const ::login_msg::MSG_RESPONSE_ECHO& response);
		
	public:
		std::shared_ptr<ClientProxy> m_ptrClientProxy;

		std::map<uint64_t, std::shared_ptr<MockRole>> m_mockRole;
	};


	using TestServerMgrSingleton = ThreadSafeSingleton<TestServerMgr>;
}
