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

	class DBProxyMgr
	{
	public:
		void init();
		void start();
		void exit();

	public:

	private:
	};

	using DBProxyMgrSingleton = ThreadSafeSingleton<DBProxyMgr>;
}
