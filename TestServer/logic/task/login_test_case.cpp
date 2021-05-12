#include "login_test_case.h"

#include "../../../SharedDir/opcodes.h"

namespace APie {
	using namespace std::placeholders;

	LoginTestCase::LoginTestCase(MockRole& role, uint32_t type) :
		TestCase(role, type)
	{

	}

	void LoginTestCase::setUp()
	{
		::pubsub::LOGIC_CMD newMsg;
		newMsg.set_cmd("account_login");

		auto ptrAdd = newMsg.add_params();
		*ptrAdd = std::to_string(this->getRole().getRoleId());

		this->getRole().pushMsg(newMsg);

		auto bindCb = std::bind(&LoginTestCase::pendingNotify_OP_MSG_RESPONSE_CLIENT_LOGIN, this, _1, _2, _3, _4);
		m_id = this->getRole().addPendingNotify(OP_MSG_RESPONSE_CLIENT_LOGIN, bindCb);
	}

	void LoginTestCase::tearDown()
	{
		this->getRole().removePendingNotifyById(m_id);
	}

	std::shared_ptr<TestCase> LoginTestCase::createMethod(MockRole& role, uint32_t type)
	{
		return std::make_shared<LoginTestCase>(role, type);
	}

	uint32_t LoginTestCase::getFactoryType()
	{
		return ETCT_Login;
	}

	void LoginTestCase::pendingNotify_OP_MSG_RESPONSE_CLIENT_LOGIN(MockRole* ptrRole, uint64_t serialNum, uint32_t opcodes, const std::string& msg)
	{
		this->setStatus(ETestCaseStatus::ECS_SUCCESS);
	}
}

