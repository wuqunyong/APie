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

#include "mock_role.h"
#include "test_case.h"
#include "json/json.h"

namespace APie {

	class TestSuite
	{
	public:
		TestSuite(uint64_t iRoleId);

		bool init();

		uint64_t getRoleId();

		void setUp();
		void tearDown();
		void run();

		bool isComplete();

		void clear();

		Json::Value getReport();

	private:
		bool loadConfig();

	public:
		static std::shared_ptr<TestSuite> makeTestSuite(uint64_t iRoleId);

	private:
		bool m_init = false;
		uint64_t m_roleId;
		std::shared_ptr<MockRole> m_ptrRole;

		std::vector<std::shared_ptr<TestCase>> m_taskList;
		uint32_t m_runTaskIndex = 0;
	};
}
