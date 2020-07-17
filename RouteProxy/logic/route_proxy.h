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

	class RouteProxy
	{
	public:
		void init();
	};


	using RouteProxySingleton = ThreadSafeSingleton<RouteProxy>;
}
