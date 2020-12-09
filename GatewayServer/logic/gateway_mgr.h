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

	class GatewayRole;

	class GatewayMgr
	{
	public:
		std::tuple<uint32_t, std::string> init();
		std::tuple<uint32_t, std::string> start();
		std::tuple<uint32_t, std::string> ready();
		void exit();

		std::shared_ptr<GatewayRole> findGatewayRoleById(uint64_t iRoleId);
		std::shared_ptr<GatewayRole> findGatewayRoleBySerialNum(uint64_t iSerialNum);
		bool addGatewayRole(std::shared_ptr<GatewayRole> ptrGatewayRole);

	public:
		static void onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg);

		static std::tuple<uint32_t, std::string> RPC_handleDeMultiplexerForward(const ::rpc_msg::CLIENT_IDENTIFIER& client, const ::rpc_msg::PRC_DeMultiplexer_Forward_Args& request);

		static void handleDefaultOpcodes(uint64_t serialNum, uint32_t opcodes, const std::string& msg);

		static void handleRequestClientLogin(uint64_t iSerialNum, const ::login_msg::MSG_REQUEST_CLIENT_LOGIN& request);

	private:
		std::map<uint64_t, std::shared_ptr<GatewayRole>> m_serialNumMap;
		std::map<uint64_t, uint64_t> m_roleIdMapSerialNum;
	};

	using GatewayMgrSingleton = ThreadSafeSingleton<GatewayMgr>;
}
