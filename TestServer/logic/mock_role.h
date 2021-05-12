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
#include "../../PBMsg/BusinessMsg/login_msg.pb.h"


namespace APie {

	class MockRole;

	struct PendingResponse
	{
		using ResponseCB = std::function<void(MockRole* ptrRole, uint64_t serialNum, uint32_t opcodes, const std::string& msg)>;

		uint32_t id = 0;

		time_t request_time = 0;
		uint32_t request_opcode = 0;

		time_t response_time = 0;
		uint32_t response_opcode = 0;

		ResponseCB cb;
		uint32_t timeout = 1000;
		uint64_t expired_at_ms;
	};

	struct PendingNotify
	{
		using ResponseCB = std::function<void(MockRole* ptrRole, uint64_t serialNum, uint32_t opcodes, const std::string& msg)>;

		uint32_t id = 0;
		uint32_t response_opcode = 0;

		ResponseCB cb;
		uint32_t timeout = 1000;
		uint64_t expired_at_ms;
	};

	class MockRole : public std::enable_shared_from_this<MockRole>
	{
	public:
		using HandlerCb = std::function<void(::pubsub::LOGIC_CMD& msg)>;
		using HandleResponseCB = std::function<void(MockRole* ptrRole, uint64_t serialNum, uint32_t opcodes, const std::string& msg)>;

		enum ConnectTarget
		{
			CT_None = 0,
			CT_Login = 1,
			CT_Gateway = 2,
		};

		MockRole(uint64_t iRoleId);
		~MockRole();

		void setUp();
		void tearDown();

		void start();

		uint64_t getRoleId();
		void processCmd();
		void addTimer(uint64_t interval);

		void disableCmdTimer();

		void clearMsg();
		void pushMsg(::pubsub::LOGIC_CMD& msg);

		bool addHandler(const std::string& name, HandlerCb cb);
		HandlerCb findHandler(const std::string& name);

		bool addResponseHandler(uint32_t opcodes, HandleResponseCB cb);
		HandleResponseCB findResponseHandler(uint32_t opcodes);
		void clearResponseHandler();

		void handleResponse(uint64_t serialNum, uint32_t opcodes, const std::string& msg);

		void setPauseProcess(bool flag);

		void addWaitResponse(uint32_t iOpcode, uint32_t iNeedCheck);
		void removeWaitResponse(uint32_t iOpcode);


		uint32_t addPendingResponse(uint32_t response, uint32_t request, HandleResponseCB cb = nullptr, uint32_t timeout = 3000);
		std::optional<PendingResponse> findPendingResponse(uint32_t response);
		void removePendingResponseById(uint32_t id);
		void clearPendingResponse();

		void handlePendingResponse(uint64_t serialNum, uint32_t opcodes, const std::string& msg);

		

		uint32_t addPendingNotify(uint32_t response, HandleResponseCB cb, uint32_t timeout = 3000);
		std::optional<PendingNotify> findPendingNotify(uint32_t response);
		void removePendingNotifyById(uint32_t id);
		void clearPendingNotify();

		void handlePendingNotify(uint64_t serialNum, uint32_t opcodes, const std::string& msg);


		bool hasTimeout(uint64_t iCurMS);

		std::map<std::tuple<uint32_t, uint32_t>, std::vector<uint64_t>>& getReplyDelay();

	private:
		void handleMsg(::pubsub::LOGIC_CMD& msg);

		void handleAccountLogin(::pubsub::LOGIC_CMD& msg);

		//void handleLogin(::pubsub::LOGIC_CMD& msg);
		void handleEcho(::pubsub::LOGIC_CMD& msg);
		void handleLogout(::pubsub::LOGIC_CMD& msg);

		void handle_MSG_RESPONSE_ACCOUNT_LOGIN_L(uint64_t serialNum, uint32_t opcodes, const std::string& msg);
		void handle_MSG_RESPONSE_HANDSHAKE_INIT(uint64_t serialNum, uint32_t opcodes, const std::string& msg);
		void handle_MSG_RESPONSE_HANDSHAKE_ESTABLISHED(uint64_t serialNum, uint32_t opcodes, const std::string& msg);
		


		void sendMsg(uint32_t iOpcode, const ::google::protobuf::Message& msg);

	public:
		static std::shared_ptr<MockRole> createMockRole(uint64_t iRoleId);

		static bool registerPbOpcodeName(uint32_t iOpcode, const std::string& sName);
		static std::optional<std::string> getPbNameByOpcode(uint32_t iOpcode);

		uint64_t m_account_id;
		std::string m_session_key;

		std::string m_clientRandom;
		std::string m_sharedKey;

	private:
		uint32_t m_id = 0;
		uint64_t m_iRoleId;
		std::shared_ptr<ClientProxy> m_clientProxy;
		Event::TimerPtr m_cmdTimer;

		ConnectTarget m_target = CT_None;

		bool m_bInit = false;
		uint64_t m_iCurIndex = 0;
		std::vector<::pubsub::LOGIC_CMD> m_configCmd;

		bool m_bPauseProcess = false;

		std::map<std::string, HandlerCb> m_cmdHandler;
		std::map<uint32_t, HandleResponseCB> m_responseHandler;

		std::map<uint32_t, uint32_t> m_waitResponse; // key:opcode, value:need check status_code

		std::list<PendingResponse> m_pendingResponse;
		std::list<PendingNotify> m_pendingNotify;
		

		std::map<std::tuple<uint32_t, uint32_t>, std::vector<uint64_t>>  m_replyDelay;  // key: request-response, value:delay(ms)

		static std::map<uint32_t, std::string> s_pbReflect;
	};
}
