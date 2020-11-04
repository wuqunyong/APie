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
		using HandlerCb = std::function<void(::pubsub::LOGIC_CMD& msg)>;

		MockRole(uint64_t iRoleId);
		~MockRole();

		void setUp();
		void tearDown();
		void start();

		uint64_t getRoleId();
		void processCmd();
		void addTimer(uint64_t interval);

		void clearMsg();
		void pushMsg(::pubsub::LOGIC_CMD& msg);

		bool addHandler(const std::string& name, HandlerCb cb);
		HandlerCb findHandler(const std::string& name);

	private:
		void handleMsg(::pubsub::LOGIC_CMD& msg);

		void handleLogin(::pubsub::LOGIC_CMD& msg);
		void handleLogout(::pubsub::LOGIC_CMD& msg);

	public:
		static std::shared_ptr<MockRole> createMockRole(uint64_t iRoleId);

	private:
		uint64_t m_iRoleId;
		std::shared_ptr<ClientProxy> m_clientProxy;
		Event::TimerPtr m_cmdTimer;

		uint64_t m_iCurIndex = 0;
		std::vector<::pubsub::LOGIC_CMD> m_configCmd;

		std::map<std::string, HandlerCb> m_cmdHander;
	};
}
