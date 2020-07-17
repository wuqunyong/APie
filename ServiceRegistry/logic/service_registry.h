#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

#include "../../PBMsg/service_discovery.pb.h"
#include "../../LibAPie/apie.h"

namespace APie {

	class ServiceRegistry
	{
	public:
		void init();

	public:
		static void handleRequestAddInstance(uint64_t iSerialNum, ::service_discovery::MSG_REQUEST_ADD_INSTANCE response);
	};


	typedef APie::ThreadSafeSingleton<ServiceRegistry> ServiceRegistrySingleton;
}
