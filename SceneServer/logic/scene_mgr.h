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

	class SceneMgr
	{
	public:
		void init();

	public:

	private:
	};

	using SceneMgrSingleton = ThreadSafeSingleton<SceneMgr>;
}
