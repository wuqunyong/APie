#include "test_case.h"


namespace APie {
	TestCase::TestCase(MockRole& role, uint32_t type) :
		m_role(role),
		m_type(type)
	{

	}

	void TestCase::init(uint32_t count, uint32_t interval)
	{
		m_loopCount = count;
		m_loopInterval = interval;
	}

	TestCase::~TestCase()
	{

	}

	void TestCase::setUp()
	{

	}

	void TestCase::tearDown()
	{

	}

	void TestCase::run()
	{
		m_role.processCmd();
	}

	void TestCase::setStatus(ETestCaseStatus status)
	{
		m_status = status;
	}

	ETestCaseStatus TestCase::getStatus()
	{
		return m_status;
	}

	MockRole& TestCase::getRole()
	{
		return m_role;
	}

	void TestCase::addCompleteLoop()
	{
		m_completeLoop += 1;
	}

	bool TestCase::isComplete()
	{
		return m_completeLoop >= m_loopCount;
	}

	void TestCase::addSuccessCount()
	{
		m_successCount++;
	}

	uint32_t TestCase::getSuccessCount()
	{
		return m_successCount;
	}

	void TestCase::addFailureCount()
	{
		m_failureCount++;
	}

	uint32_t TestCase::getFailureCount()
	{
		return m_failureCount;
	}

	bool TestCase::hasTimeout(uint64_t iCurMS)
	{
		return m_role.hasTimeout(iCurMS);
	}

	bool TestCase::expiredLoopTime(uint64_t iCurMS)
	{
		if (m_nextLoopTime == 0)
		{
			m_nextLoopTime = iCurMS;
		}

		if (iCurMS < m_nextLoopTime)
		{
			return false;
		}

		m_nextLoopTime = iCurMS + m_loopInterval;
		return true;
	}

	std::map<uint32_t, TestCaseFactory::TCreateMethod> TestCaseFactory::s_methods;

	bool TestCaseFactory::registerFactory(uint32_t type, TCreateMethod funcCreate)
	{
		if (auto it = s_methods.find(type); it == s_methods.end())
		{ 
			s_methods[type] = funcCreate;
			return true;
		}

		PIE_LOG("TestCaseFactory/registerFactory", PIE_CYCLE_DAY, PIE_ERROR, "%d: duplicate register", type);
		return false;
	}

	std::shared_ptr<TestCase> TestCaseFactory::create(MockRole& role, uint32_t type)
	{
		if (auto it = s_methods.find(type); it != s_methods.end())
		{
			return it->second(role, type);
		}

		return nullptr;
	}


}

