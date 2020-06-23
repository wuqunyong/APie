#include "apie.h"

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <locale.h>
#include <openssl/des.h>
#include <algorithm>

#include "../PBMsg/login_msg.pb.h"

class TestPbClass
{
public:
	static void HandleFun1(uint64_t serialNum, ::login_msg::MSG_CLIENT_LOGINTOL msg) {
		std::cout << "serialNum:" << serialNum << std::endl;
		msg.PrintDebugString();


		::login_msg::MSG_CLIENT_LOGINTOL response;
		response.set_user_id(msg.user_id());

		APie::Network::OutputStream::sendMsg(APie::ConnetionType::CT_SERVER, serialNum, 1101, response);
	};
};

std::tuple<uint32_t, std::string> initHook()
{
	APie::Api::OpcodeHandlerSingleton::get().server.bind(1100, TestPbClass::HandleFun1, ::login_msg::MSG_CLIENT_LOGINTOL());

	auto replyCb = [](uint64_t serialNum, ::login_msg::MSG_CLIENT_LOGINTOL msg) {
		std::cout << "serialNum:" << serialNum << ",msg:" << msg.DebugString() << std::endl;

		auto ptrClint = APie::ClientProxy::findClient(serialNum);
		if (ptrClint == nullptr)
		{
			return;
		}

		::login_msg::MSG_CLIENT_LOGINTOL request;
		request.set_user_id(time(NULL));
		request.set_session_key("hello");

		//ptrClint->sendMsg(1100, request);
	};
	APie::Api::OpcodeHandlerSingleton::get().client.bind(1101, replyCb, ::login_msg::MSG_CLIENT_LOGINTOL());

	return std::make_tuple(0, "");
}

std::tuple<uint32_t, std::string> initHook1()
{
	return std::make_tuple(0, "");
}

std::tuple<uint32_t, std::string> initHook2()
{
	return std::make_tuple(0, "");
}

std::tuple<uint32_t, std::string> startHook()
{
	for (const auto& item : APie::CtxSingleton::get().yamlNode()["clients"])
	{
		std::string ip = item["address"]["socket_address"]["address"].as<std::string>();
		uint16_t port = item["address"]["socket_address"]["port_value"].as<uint16_t>();
		uint16_t type = item["address"]["socket_address"]["type"].as<uint16_t>();

		auto ptrClient = APie::ClientProxy::createClientProxy();
		auto connectCb = [](std::shared_ptr<APie::ClientProxy> self, uint32_t iResult) {
			if (iResult == 0)
			{
				::login_msg::MSG_CLIENT_LOGINTOL msg;
				msg.set_user_id(100);
				msg.set_session_key("hello");

				self->sendMsg(1100, msg);
				self->addReconnectTimer(30000);
			}
			return true;
		};
		ptrClient->connect(ip, port, static_cast<APie::ProtocolType>(type), connectCb);

		auto heartbeatCb = [](APie::ClientProxy *ptrClient) {
			std::cout << "curTime:" << time(NULL) << std::endl;
			ptrClient->addHeartbeatTimer(1000);
		};
		ptrClient->setHeartbeatCb(heartbeatCb);
		ptrClient->addHeartbeatTimer(1000);
		ptrClient.reset();
	}


	return std::make_tuple(0, "");
}


std::tuple<uint32_t, std::string> exitHook()
{
	return std::make_tuple(0, "");
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fatalExit("usage: exe <ConfFile>");
	}

	std::string configFile = argv[1];

	APie::Hook::HookRegistrySingleton::get().appendHook(APie::Hook::HookPoint::HP_Init, initHook1, 1);
	APie::Hook::HookRegistrySingleton::get().appendHook(APie::Hook::HookPoint::HP_Init, initHook);
	APie::Hook::HookRegistrySingleton::get().appendHook(APie::Hook::HookPoint::HP_Init, initHook2, 2);

	APie::Hook::HookRegistrySingleton::get().appendHook(APie::Hook::HookPoint::HP_Start, startHook);

	APie::Hook::HookRegistrySingleton::get().appendHook(APie::Hook::HookPoint::HP_Exit, exitHook);

	APie::CtxSingleton::get().init(configFile);
	APie::CtxSingleton::get().start();
	APie::CtxSingleton::get().waitForShutdown();
    return 0;
}
