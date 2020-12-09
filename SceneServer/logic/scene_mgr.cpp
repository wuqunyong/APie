#include "scene_mgr.h"

#include "../../PBMsg/opcodes.pb.h"


namespace APie {

std::tuple<uint32_t, std::string> SceneMgr::init()
{
	APie::RPC::rpcInit();
	APie::RPC::RpcServerSingleton::get().registerOpcodes<::rpc_msg::PRC_Multiplexer_Forward_Args>(rpc_msg::RPC_Multiplexer_Forward, SceneMgr::RPC_handleMultiplexerForward);

	APie::Api::ForwardHandlerSingleton::get().server.bind(::opcodes::OP_MSG_REQUEST_ECHO, SceneMgr::Forward_handlEcho, ::login_msg::MSG_REQUEST_ECHO::default_instance());

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> SceneMgr::start()
{
	APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> SceneMgr::ready()
{
	std::stringstream ss;
	ss << "Server Ready!";
	std::cout << ss.str() << std::endl;
	ASYNC_PIE_LOG("ServerStatus", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

void SceneMgr::exit()
{

}

std::tuple<uint32_t, std::string> SceneMgr::RPC_handleMultiplexerForward(const ::rpc_msg::CLIENT_IDENTIFIER& client, ::rpc_msg::PRC_Multiplexer_Forward_Args request)
{
	request.mutable_role_id()->set_channel_serial_num(client.channel_serial_num());

	auto optionalData = Api::ForwardHandlerSingleton::get().server.getType(request.opcodes());
	if (!optionalData)
	{
		std::stringstream ss;
		ss << "Unregister|" << "opcodes:" << request.opcodes();
		return std::make_tuple(::rpc_msg::CODE_OpcodeUnregister, ss.str());
	}

	std::string sType = optionalData.value();
	auto ptrMsg = Api::PBHandler::createMessage(sType);
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
}

void SceneMgr::Forward_handlEcho(::rpc_msg::RoleIdentifier roleIdentifier, ::login_msg::MSG_REQUEST_ECHO request)
{
	PIE_LOG("SceneMgr/Forward_handlEcho", PIE_CYCLE_DAY, PIE_NOTICE, "%s", request.DebugString().c_str());

	uint64_t iCurMS = Ctx::getNowMilliseconds();

	::login_msg::MSG_RESPONSE_ECHO response;
	response.set_value1(iCurMS);
	response.set_value2(request.value2() + "|response");
	Network::OutputStream::sendMsgToUserByGateway(roleIdentifier, opcodes::OP_MSG_RESPONSE_ECHO, response);
}

}

