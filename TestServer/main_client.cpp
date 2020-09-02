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

		APie::Network::OutputStream::sendMsg(serialNum, 1101, response, APie::ConnetionType::CT_SERVER);
	};
};

std::tuple<uint32_t, std::string> initHook()
{
	APie::Api::OpcodeHandlerSingleton::get().server.bind(1100, TestPbClass::HandleFun1, ::login_msg::MSG_CLIENT_LOGINTOL());

	auto replyCb = [](uint64_t serialNum, ::login_msg::MSG_CLIENT_LOGINTOL msg) {
		std::cout << "serialNum:" << serialNum << ",msg:" << msg.ShortDebugString() << std::endl;

		auto ptrClint = APie::ClientProxy::findClient(serialNum);
		if (ptrClint == nullptr)
		{
			return;
		}

		::login_msg::MSG_CLIENT_LOGINTOL request;
		request.set_user_id(time(NULL));
		request.set_session_key("hello");

		//ptrClint->sendMsg(1100, request);

		::rpc_msg::CONTROLLER cntl;
		cntl.set_serial_num(serialNum);

		::rpc_msg::CHANNEL server;
		server.set_type(1);
		server.set_id(1);

		auto rpcCB = [](const rpc_msg::STATUS& status, const std::string& replyData)
		{
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}
		};
		APie::RPC::RpcClientSingleton::get().call(cntl, server, ::rpc_msg::PRC_None, request, rpcCB);
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
	APie::RPC::init();

	auto rpcCB = [](const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args) -> std::tuple<uint32_t, std::string> {
		::login_msg::MSG_CLIENT_LOGINTOL request;
		if (!request.ParseFromString(args))
		{
			return std::make_tuple(::rpc_msg::CODE_Unregister, "");;
		}

		return std::make_tuple(::rpc_msg::CODE_Ok, "aaaaaaaaaaaaaaa");;
	};
	APie::RPC::RpcServerSingleton::get().registerOpcodes(rpc_msg::PRC_None, rpcCB);

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
		auto connectCb = [](APie::ClientProxy* ptrClient, uint32_t iResult) {
			if (iResult == 0)
			{
				::login_msg::MSG_CLIENT_LOGINTOL msg;
				msg.set_user_id(100);
				msg.set_session_key("hello");

				ptrClient->sendMsg(1100, msg);
				ptrClient->addReconnectTimer(30000);
			}
			return true;
		};
		ptrClient->connect(ip, port, static_cast<APie::ProtocolType>(type), connectCb);

		auto heartbeatCb = [](APie::ClientProxy *ptrClient) {
			ptrClient->addHeartbeatTimer(30000);
		};
		ptrClient->setHeartbeatCb(heartbeatCb);
		ptrClient->addHeartbeatTimer(1000);
		ptrClient.reset();
	}


	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_LogicCmd, [](uint64_t topic, ::google::protobuf::Message& msg) {
		auto& refMsg1 = dynamic_cast<::pubsub::LOGIC_CMD&>(msg);
		std::cout << "topic:" << topic << ",refMsg1:" << refMsg1.ShortDebugString() << std::endl;
	});
	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_LogicCmd, [](uint64_t topic, ::google::protobuf::Message& msg) {
		auto& refMsg2 = dynamic_cast<::pubsub::LOGIC_CMD&>(msg);
		std::cout << "topic:" << topic << ",refMsg2:" << refMsg2.ShortDebugString() << std::endl;
	});

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

	//auto ptrTest1 = [](uint64_t topic, ::google::protobuf::Message& msg) {
	//	auto& refMsg = dynamic_cast<::login_msg::MSG_CLIENT_LOGINTOL&>(msg);
	//	std::cout << "sub:" << refMsg.ShortDebugString();

	//	auto userId = refMsg.user_id() + 111;
	//	refMsg.set_user_id(userId);
	//};
	//APie::PubSubSingleton::get().subscribe(1, ptrTest1);

	//auto ptrTest2 = [](uint64_t topic, ::google::protobuf::Message& msg) {
	//	auto& refMsg = dynamic_cast<::login_msg::MSG_CLIENT_LOGINTOL&>(msg);
	//	std::cout << "sub:" << refMsg.ShortDebugString();
	//};
	//APie::PubSubSingleton::get().subscribe(1, ptrTest2);
	//uint64_t id = APie::PubSubSingleton::get().subscribe(1, ptrTest2);
	//APie::PubSubSingleton::get().unregister(1, id);


	//::login_msg::MSG_CLIENT_LOGINTOL msg;
	//msg.set_user_id(100);
	//msg.set_session_key("hello");
	//APie::PubSubSingleton::get().publish(1, msg);
	//APie::PubSubSingleton::get().publish(2, msg);

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
