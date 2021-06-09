#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>
#include <memory>
#include <set>

#include "apie.h"

#include "test_suite.h"

namespace APie {

	class TestRunner
	{
	public:
		TestRunner();
		
		void init();
		void run();

	private:
		bool loadConfig();

	private:
		uint64_t m_start = 0;
		uint64_t m_stop = 0;
		uint32_t m_rampUpIntervalMs = 1000;
		uint32_t m_rampUpNums = 100;

		uint64_t m_startTime = 0;
		uint64_t m_activePos = 0;
		std::map<uint64_t, std::shared_ptr<TestSuite>> m_activeSuite;
		std::map<uint64_t, std::shared_ptr<TestSuite>> m_doneSuite;

		uint64_t m_nextRampUpTimes = 0;

		bool m_init = false;
		bool m_enable = false;
		bool m_done = false;
		Event::TimerPtr m_runTimer;
	};


	using TestRunnerMgrSingleton = ThreadSafeSingleton<TestRunner>;
}
