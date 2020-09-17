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
		void init();
		void start();
		void exit();

	public:

	private:
		std::map<uint64_t, uint64_t> m_serialNumRoleId;
	};

	using GatewayMgrSingleton = ThreadSafeSingleton<GatewayMgr>;
}
