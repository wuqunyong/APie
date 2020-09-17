#include "gateway_mgr.h"

namespace APie {

void GatewayMgr::init()
{
	APie::RPC::rpcInit();

	auto handleServerFunc = [](uint64_t serialNum, uint32_t opcodes, const std::string& msg) {
		::rpc_msg::PRC_Multiplexer_Forward_Args args;
		args.set_opcodes(opcodes);
		args.set_body_msg(msg);

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_Scene_Server);
		server.set_id(1);

		auto rpcCB = [](const rpc_msg::STATUS& status, const std::string& replyData)
		{
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}
		};
		APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::PRC_Multiplexer_Forward, args, rpcCB);
	};
	Api::OpcodeHandlerSingleton::get().server.setDefaultFunc(handleServerFunc);
}

void GatewayMgr::start()
{
}

void GatewayMgr::exit()
{

}

}

