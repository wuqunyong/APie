#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>
#include <memory>

#include "apie.h"
#include "../../PBMsg/login_msg.pb.h"


namespace APie {

	class MockRole : public std::enable_shared_from_this<MockRole>
	{
	public:
		MockRole(uint64_t iRoleId);

		void setUp();
		void tearDown();

		void processCmd();
		void addTimer(uint64_t interval);
		
	public:
		static std::shared_ptr<MockRole> createMockRole(uint64_t iRoleId);

	private:
		uint64_t m_iRoleId;
		std::shared_ptr<ClientProxy> m_clientProxy;
		Event::TimerPtr m_cmdTimer;

		uint64_t m_iCurIndex;
		std::vector<::pubsub::LOGIC_CMD> m_configCmd;
	};
}
