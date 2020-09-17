#include "scene_mgr.h"

namespace APie {

void SceneMgr::init()
{
	APie::RPC::rpcInit();

	auto rpcCB = [](const ::rpc_msg::CLIENT_IDENTIFIER& client, const std::string& args) -> std::tuple<uint32_t, std::string> {
		::rpc_msg::PRC_Multiplexer_Forward_Args request;
		if (!request.ParseFromString(args))
		{
			return std::make_tuple(::rpc_msg::CODE_ParseError, "");
		}

		return std::make_tuple(::rpc_msg::CODE_Ok, "forward success");
	};
	APie::RPC::RpcServerSingleton::get().registerOpcodes(rpc_msg::PRC_Multiplexer_Forward, rpcCB);
}

void SceneMgr::start()
{

}

void SceneMgr::exit()
{

}

}

