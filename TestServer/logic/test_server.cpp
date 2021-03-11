#include "test_server.h"

#include "json/json.h"

#include "../../LibAPie/redis_driver/redis_client.h"
#include "../../SharedDir/opcodes.h"

namespace APie {


std::tuple<uint32_t, std::string> TestServerMgr::init()
{
	auto bResult = APie::CtxSingleton::get().checkIsValidServerType({ common::EPT_Test_Client });
	if (!bResult)
	{
		return std::make_tuple(Hook::HookResult::HR_Error, "invalid Type");
	}

	APie::RPC::rpcInit();

	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_LogicCmd, TestServerMgr::onLogicCommnad);
	
	Api::OpcodeHandlerSingleton::get().client.setDefaultFunc(TestServerMgr::handleDefaultOpcodes);

	MockRole::registerPbOpcodeName(OP_MSG_RESPONSE_ACCOUNT_LOGIN_L, "login_msg.MSG_RESPONSE_ACCOUNT_LOGIN_L");
	MockRole::registerPbOpcodeName(OP_MSG_RESPONSE_CLIENT_LOGIN, "login_msg.MSG_RESPONSE_CLIENT_LOGIN");
	MockRole::registerPbOpcodeName(OP_MSG_RESPONSE_ECHO, "login_msg.MSG_RESPONSE_ECHO");
	MockRole::registerPbOpcodeName(OP_MSG_RESPONSE_HANDSHAKE_INIT, "login_msg.MSG_RESPONSE_HANDSHAKE_INIT");
	MockRole::registerPbOpcodeName(OP_MSG_RESPONSE_HANDSHAKE_ESTABLISHED, "login_msg.MSG_RESPONSE_HANDSHAKE_ESTABLISHED");

	//Api::OpcodeHandlerSingleton::get().client.bind(::APie::OP_MSG_RESPONSE_CLIENT_LOGIN, TestServerMgr::handleResponseClientLogin, ::login_msg::MSG_RESPONSE_CLIENT_LOGIN::default_instance());
	//Api::OpcodeHandlerSingleton::get().client.bind(::APie::OP_MSG_RESPONSE_ECHO, TestServerMgr::handleResponseEcho, ::login_msg::MSG_RESPONSE_ECHO::default_instance());


	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> TestServerMgr::start()
{
	std::string ip = APie::CtxSingleton::get().yamlAs<std::string>({ "clients", "socket_address", "address" }, "");
	uint16_t port = APie::CtxSingleton::get().yamlAs<uint16_t>({ "clients", "socket_address", "port_value" }, 0);
	uint16_t type = APie::CtxSingleton::get().yamlAs<uint16_t>({ "clients", "socket_address", "type" }, 0);
	uint32_t maskFlag = APie::CtxSingleton::get().yamlAs<uint16_t>({ "clients", "socket_address", "mask_flag" }, 0);

	m_ptrClientProxy = APie::ClientProxy::createClientProxy();
	auto connectCb = [](APie::ClientProxy* ptrClient, uint32_t iResult) {
		if (iResult == 0)
		{
			APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);
		}
		return true;
	};
	m_ptrClientProxy->connect(ip, port, static_cast<APie::ProtocolType>(type), maskFlag, connectCb);
	m_ptrClientProxy->addReconnectTimer(60000);

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> TestServerMgr::ready()
{
	std::stringstream ss;
	ss << "Server Ready!";
	std::cout << ss.str() << std::endl;
	ASYNC_PIE_LOG("ServerStatus", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

void TestServerMgr::exit()
{
}

void TestServerMgr::addMockRole(std::shared_ptr<MockRole> ptrMockRole)
{
	auto iRoleId = ptrMockRole->getRoleId();
	m_mockRole[iRoleId] = ptrMockRole;
}

std::shared_ptr<MockRole> TestServerMgr::findMockRole(uint64_t iRoleId)
{
	auto findIte = m_mockRole.find(iRoleId);
	if (findIte == m_mockRole.end())
	{
		return nullptr;
	}

	return findIte->second;
}

void TestServerMgr::removeMockRole(uint64_t iRoleId)
{
	m_mockRole.erase(iRoleId);
}

void TestServerMgr::addSerialNumRole(uint64_t iSerialNum, uint64_t iRoleId)
{
	m_serialNumRole[iSerialNum] = iRoleId;
}

std::optional<uint64_t> TestServerMgr::findRoleIdBySerialNum(uint64_t iSerialNum)
{
	auto findIte = m_serialNumRole.find(iSerialNum);
	if (findIte == m_serialNumRole.end())
	{
		return std::nullopt;
	}

	return findIte->second;
}

void TestServerMgr::removeSerialNum(uint64_t iSerialNum)
{
	m_serialNumRole.erase(iSerialNum);
}

void TestServerMgr::onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg)
{
	auto& command = dynamic_cast<::pubsub::LOGIC_CMD&>(msg);

	if (command.cmd() == "login")
	{
		if (command.params_size() < 2)
		{
			return;
		}

		::login_msg::MSG_REQUEST_CLIENT_LOGIN request;
		request.set_user_id(std::stoull(command.params()[0]));
		request.set_version(std::stoi(command.params()[1]));
		request.set_session_key(command.params()[2]);

		TestServerMgrSingleton::get().m_ptrClientProxy->sendMsg(::APie::OP_MSG_REQUEST_CLIENT_LOGIN, request);

		std::cout << "send|iSerialNum:" << TestServerMgrSingleton::get().m_ptrClientProxy->getSerialNum() << "|request:" << request.ShortDebugString() << std::endl;
	}
	else if (command.cmd() == "echo")
	{
		if (command.params_size() < 1)
		{
			return;
		}

		uint64_t iCurMS = Ctx::getCurMilliseconds();

		::login_msg::MSG_REQUEST_ECHO request;
		request.set_value1(iCurMS);
		request.set_value2(command.params()[0]);

		TestServerMgrSingleton::get().m_ptrClientProxy->sendMsg(::APie::OP_MSG_REQUEST_ECHO, request);

		std::cout << "send|iSerialNum:" << TestServerMgrSingleton::get().m_ptrClientProxy->getSerialNum() << "|request:" << request.ShortDebugString() << std::endl;
	}
	else if (command.cmd() == "redis")
	{
		if (command.params_size() < 2)
		{
			return;
		}


		auto key = std::make_tuple(1, 1);
		std::string sTable = command.params()[0];  //command.params()[0];
		std::string field = command.params()[1];


		if (sTable == "hs_player_summary")
		{
			auto redisClient = RedisClientFactorySingleton::get().getClient(key);
			if (redisClient == nullptr)
			{
				return;
			}

			if (!redisClient->client().is_connected())
			{
				return;
			}

			auto cb = [sTable](cpp_redis::reply &reply) {
				if (reply.is_error())
				{
					return;
				}


				if (reply.is_null())
				{
					return;
				}

				if (!reply.is_bulk_string())
				{
					return;
				}

				::role_msg::ROLE_SUMMARY summary;

				std::string content = reply.as_string();
				if (!summary.ParseFromString(content))
				{
					return;
				}

				std::cout << summary.DebugString() << std::endl;
			};
			redisClient->client().hget(sTable, field, cb);
			redisClient->client().commit();
		}
		else if (sTable == "order_info")
		{
			field = command.params()[1] + "|" + command.params()[2];

			auto redisClient = RedisClientFactorySingleton::get().getClient(key);
			if (redisClient == nullptr)
			{
				return;
			}

			if (!redisClient->client().is_connected())
			{
				return;
			}

			auto cb = [sTable](cpp_redis::reply &reply) {
				if (reply.is_error())
				{
					return;
				}


				if (reply.is_null())
				{
					return;
				}

				if (!reply.is_bulk_string())
				{
					return;
				}

				//::gm_msg::ORDER_INFO order;

				//std::string content = reply.as_string();
				//if (!order.ParseFromString(content))
				//{
				//	return;
				//}

				//std::cout << order.DebugString() << std::endl;
			};
			redisClient->client().hget(sTable, field, cb);
			redisClient->client().commit();
		}
		else
		{

		}

	}
	else if (command.cmd() == "client")
	{
		if (command.params_size() < 2)
		{
			std::cout << "invalid params" << std::endl;
			return;
		}

		uint64_t iRoleId = std::stoull(command.params()[0]);
		auto ptrMockRole = TestServerMgrSingleton::get().findMockRole(iRoleId);
		if (ptrMockRole == nullptr)
		{
			if (command.params()[1] == "account_login")
			{
				ptrMockRole = MockRole::createMockRole(iRoleId);
				ptrMockRole->start();

				TestServerMgrSingleton::get().addMockRole(ptrMockRole);
			}
			else
			{
				std::cout << "not login|account:" << iRoleId << std::endl;
				return;
			}
		}

		::pubsub::LOGIC_CMD newMsg;
		for (int i = 1; i < command.params().size(); i++)
		{
			if (i == 1)
			{
				newMsg.set_cmd(command.params()[i]);
			}
			else
			{
				auto ptrAdd = newMsg.add_params();
				*ptrAdd = command.params()[i];
			}
		}
		ptrMockRole->pushMsg(newMsg);
	}

}

//void TestServerMgr::handleResponseClientLogin(uint64_t iSerialNum, const ::login_msg::MSG_RESPONSE_CLIENT_LOGIN& response)
//{
//	std::cout << "recv|iSerialNum:" << iSerialNum << "|response:" << response.ShortDebugString() << std::endl;
//}
//
//void TestServerMgr::handleResponseEcho(uint64_t iSerialNum, const ::login_msg::MSG_RESPONSE_ECHO& response)
//{
//	std::cout << "recv|iSerialNum:" << iSerialNum << "|response:" << response.ShortDebugString() << std::endl;
//}

void TestServerMgr::handleDefaultOpcodes(uint64_t serialNum, uint32_t opcodes, const std::string& msg)
{
	auto iRoleIdOpt = TestServerMgrSingleton::get().findRoleIdBySerialNum(serialNum);
	if (!iRoleIdOpt.has_value())
	{
		return;
	}

	uint64_t iRoleId = iRoleIdOpt.value();
	auto ptrMockRole = TestServerMgrSingleton::get().findMockRole(iRoleId);
	if (ptrMockRole == nullptr)
	{
		return;
	}

	ptrMockRole->handleResponse(serialNum, opcodes, msg);
}

}

