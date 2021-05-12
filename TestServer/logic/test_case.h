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


namespace APie {

	enum ETestCaseType
	{
		ETCT_Login = 1,
		ETCT_Echo,
		ETCT_Logout,
	};

	enum class ETestCaseStatus
	{
		ECS_INVALID = 0,
		ECS_RUNNING,
		ECS_SUCCESS,
		ECS_FAILURE,
	};

	class TestCase
	{
	public:

		TestCase(MockRole& role, uint32_t type);
		virtual ~TestCase();

		void init(uint32_t count, uint32_t interval);

		virtual void setUp();
		virtual void tearDown();
		virtual void run();

		void setStatus(ETestCaseStatus status);
		ETestCaseStatus getStatus();

		MockRole& getRole();
		void addCompleteLoop();
		bool isComplete();

		uint32_t getType();
		uint32_t getCount();

		void addSuccessCount();
		uint32_t getSuccessCount();

		void addFailureCount();
		uint32_t getFailureCount();
		
		bool hasTimeout(uint64_t iCurMS);

		bool expiredLoopTime(uint64_t iCurMS);

	private:
		MockRole& m_role;

		uint32_t m_type = 0;
		uint32_t m_loopCount = 1;
		uint32_t m_loopInterval = 10;
		uint32_t m_completeLoop = 0;
		uint32_t m_successCount = 0;
		uint32_t m_failureCount = 0;
		ETestCaseStatus m_status = ETestCaseStatus::ECS_INVALID;

		uint64_t m_nextLoopTime = 0;

	};


	class TestCaseFactory
	{
	public:
		using TCreateMethod = std::shared_ptr<TestCase>(*)(MockRole& role, uint32_t type);

	public:
		TestCaseFactory() = delete;

		static bool registerFactory(uint32_t type, TCreateMethod funcCreate);

		static std::shared_ptr<TestCase> create(MockRole& role, uint32_t type);

	private:
		static std::map<uint32_t, TCreateMethod> s_methods;
	};


}
