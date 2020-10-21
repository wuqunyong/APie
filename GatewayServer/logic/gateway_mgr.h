#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

#include "apie.h"


namespace APie {

	class GatewayMgr
	{
	public:
		std::tuple<uint32_t, std::string> init();
		std::tuple<uint32_t, std::string> start();
		std::tuple<uint32_t, std::string> ready();
		void exit();

	public:
		static void onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg);

		static void handleDefaultOpcodes(uint64_t serialNum, uint32_t opcodes, const std::string& msg);

	private:
		std::map<uint64_t, uint64_t> m_serialNumRoleId;
	};

	using GatewayMgrSingleton = ThreadSafeSingleton<GatewayMgr>;
}
