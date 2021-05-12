#include "echo_test_case.h"

#include "../../../SharedDir/opcodes.h"

namespace APie {
	using namespace std::placeholders;

	EchoTestCase::EchoTestCase(MockRole& role, uint32_t type) :
		TestCase(role, type)
	{

	}

	void EchoTestCase::setUp()
	{
		time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);

		std::string sTime = asctime(timeinfo);

		::pubsub::LOGIC_CMD newMsg;
		newMsg.set_cmd("echo");

		auto ptrAdd1 = newMsg.add_params();
		*ptrAdd1 = std::to_string(rawtime);

		auto ptrAdd2 = newMsg.add_params();
		*ptrAdd2 = sTime;

		this->getRole().pushMsg(newMsg);

		auto bindCb = std::bind(&EchoTestCase::pendingNotify_OP_MSG_RESPONSE_ECHO, this, _1, _2, _3, _4);
		m_id = this->getRole().addPendingNotify(OP_MSG_RESPONSE_ECHO, bindCb);
	}

	void EchoTestCase::tearDown()
	{
		this->getRole().removePendingNotifyById(m_id);
	}

	std::shared_ptr<TestCase> EchoTestCase::createMethod(MockRole& role, uint32_t type)
	{
		return std::make_shared<EchoTestCase>(role, type);
	}

	uint32_t EchoTestCase::getFactoryType()
	{
		return ETCT_Echo;
	}

	void EchoTestCase::pendingNotify_OP_MSG_RESPONSE_ECHO(MockRole* ptrRole, uint64_t serialNum, uint32_t opcodes, const std::string& msg)
	{
		this->setStatus(ETestCaseStatus::ECS_SUCCESS);
	}
}

