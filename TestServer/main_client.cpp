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

int main(int argc, char **argv)
{
	APie::CtxSingleton::get().init();
	APie::CtxSingleton::get().start();

	APie::Api::OpcodeHandlerSingleton::get().server.bind(1100, TestPbClass::HandleFun1, ::login_msg::MSG_CLIENT_LOGINTOL());

	auto replyCb = [](uint64_t serialNum, ::login_msg::MSG_CLIENT_LOGINTOL msg) {
		std::cout << "serialNum:" << serialNum << ",msg:" << msg.DebugString() << std::endl;
	};
	APie::Api::OpcodeHandlerSingleton::get().client.bind(1101, replyCb, ::login_msg::MSG_CLIENT_LOGINTOL());
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
	ptrClient->connect("127.0.0.1", 5007, APie::ProtocolType::PT_PB, connectCb);

	auto heartbeatCb = [](APie::ClientProxy *ptrClient) {
		std::cout << "curTime:" << time(NULL) << std::endl;
		ptrClient->addHeartbeatTimer(1000);
	};
	ptrClient->setHeartbeatCb(heartbeatCb);
	ptrClient->addHeartbeatTimer(1000);
	ptrClient.reset();

	std::cin.get();
	APie::CtxSingleton::get().destroy();

	std::cin.get();
    return 0;
}
