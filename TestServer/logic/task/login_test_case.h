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
#include "../mock_role.h"
#include "../test_case.h"


namespace APie {

	class LoginTestCase : public TestCase
	{
	public:
		LoginTestCase(MockRole& role, uint32_t type);

		virtual void setUp();
		virtual void tearDown();

	public:
		void pendingNotify_OP_MSG_RESPONSE_CLIENT_LOGIN(MockRole* ptrRole, uint64_t serialNum, uint32_t opcodes, const std::string& msg);

	public:
		static std::shared_ptr<TestCase> createMethod(MockRole& role, uint32_t type);
		static uint32_t getFactoryType();

	private:
		uint32_t m_id;
	};
}
