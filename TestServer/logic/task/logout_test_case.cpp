#include "logout_test_case.h"

#include "../../../SharedDir/opcodes.h"

namespace APie {
	using namespace std::placeholders;

	LogoutTestCase::LogoutTestCase(MockRole& role, uint32_t type) :
		TestCase(role, type)
	{

	}

	void LogoutTestCase::setUp()
	{
		::pubsub::LOGIC_CMD newMsg;
		newMsg.set_cmd("logout");
		this->getRole().pushMsg(newMsg);

		auto bindCb = std::bind(&LogoutTestCase::pendingNotify_Dummy, this, _1, _2, _3, _4);
		m_id = this->getRole().addPendingNotify(0, bindCb);
	}

	void LogoutTestCase::tearDown()
	{
	}

	std::shared_ptr<TestCase> LogoutTestCase::createMethod(MockRole& role, uint32_t type)
	{
		return std::make_shared<LogoutTestCase>(role, type);
	}

	uint32_t LogoutTestCase::getFactoryType()
	{
		return ETCT_Logout;
	}

	void LogoutTestCase::pendingNotify_Dummy(MockRole* ptrRole, uint64_t serialNum, uint32_t opcodes, const std::string& msg)
	{
		this->setStatus(ETestCaseStatus::ECS_SUCCESS);
	}
}

