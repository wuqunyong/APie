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

	public:

	private:
	};

	using GatewayMgrSingleton = ThreadSafeSingleton<GatewayMgr>;
}
