#include "scene_mgr.h"

#include "../../PBMsg/login_msg.pb.h"
#include "../../PBMsg/opcodes.pb.h"


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


		auto optionalData = Api::ForwardHandlerSingleton::get().server.getType(request.opcodes());
		if (!optionalData)
		{
			std::stringstream ss;
			ss << "Unregister|" << "opcodes:" << request.opcodes();
			return std::make_tuple(::rpc_msg::CODE_OpcodeUnregister, ss.str());
		}

		std::string sType = optionalData.value();
		auto ptrMsg = Api::ForwardHandlerSingleton::get().server.createMessage(sType);
		if (ptrMsg == nullptr)
		{
			std::stringstream ss;
			ss << "Type|type:" << sType;
			return std::make_tuple(::rpc_msg::CODE_CreateMsgError, ss.str());
		}

		std::shared_ptr<::google::protobuf::Message> newMsg(ptrMsg);
		bool bResult = newMsg->ParseFromString(request.body_msg());
		if (!bResult)
		{
			std::stringstream ss;
			ss << "ParseError|" << "opcodes:" << request.opcodes();
			return std::make_tuple(::rpc_msg::CODE_ParseError, ss.str());
		}

		auto optionalFunc = Api::ForwardHandlerSingleton::get().server.getFunction(request.opcodes());
		if (!optionalFunc)
		{
			return std::make_tuple(::rpc_msg::CODE_OpcodeUnregister, "");
		}
		optionalFunc.value()(request.role_id(), newMsg.get());


		return std::make_tuple(::rpc_msg::CODE_Ok, "forward success");
	};
	APie::RPC::RpcServerSingleton::get().registerOpcodes(rpc_msg::RPC_Multiplexer_Forward, rpcCB);


	auto handleLogin = [](::rpc_msg::RoleIdentifier roleIdentifier, ::login_msg::MSG_REQUEST_CLIENT_LOGIN request) {
		PIE_LOG("startup/startup", PIE_CYCLE_DAY, PIE_NOTICE, "login|%s", request.DebugString().c_str());
	};
	APie::Api::ForwardHandlerSingleton::get().server.bind(::opcodes::OP_MSG_REQUEST_CLIENT_LOGIN, handleLogin, ::login_msg::MSG_REQUEST_CLIENT_LOGIN::default_instance());
}

void SceneMgr::start()
{

}

void SceneMgr::exit()
{

}

}

