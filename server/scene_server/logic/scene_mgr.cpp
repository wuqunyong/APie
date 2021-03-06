#include "scene_mgr.h"

#include "../../pb_msg/core/opcodes.pb.h"
#include "../../common/opcodes.h"


namespace APie {

std::tuple<uint32_t, std::string> SceneMgr::init()
{
	auto bResult = APie::CtxSingleton::get().checkIsValidServerType({ common::EPT_Scene_Server });
	if (!bResult)
	{
		return std::make_tuple(Hook::HookResult::HR_Error, "invalid Type");
	}

	// CMD
	LogicCmdHandlerSingleton::get().init();
	APie::PubSubSingleton::get().subscribe(::pubsub::PUB_TOPIC::PT_LogicCmd, SceneMgr::onLogicCommnad);


	// RPC
	APie::RPC::rpcInit();
	APie::RPC::RpcServerSingleton::get().bind(rpc_msg::RPC_Multiplexer_Forward, SceneMgr::RPC_handleMultiplexerForward);


	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> SceneMgr::start()
{
	APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Ready);

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> SceneMgr::ready()
{
	// CLIENT OPCODE
	auto& forwardHandler = APie::Api::ForwardHandlerSingleton::get();
	forwardHandler.server.bind(::APie::OP_MSG_REQUEST_ECHO, SceneMgr::Forward_handlEcho);


	std::stringstream ss;
	ss << "Server Ready!";
	std::cout << ss.str() << std::endl;
	ASYNC_PIE_LOG("ServerStatus", PIE_CYCLE_DAY, PIE_NOTICE, ss.str().c_str());

	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

void SceneMgr::exit()
{

}

void SceneMgr::onLogicCommnad(uint64_t topic, ::google::protobuf::Message& msg)
{

	auto& command = dynamic_cast<::pubsub::LOGIC_CMD&>(msg);
	auto handlerOpt = LogicCmdHandlerSingleton::get().findCb(command.cmd());
	if (!handlerOpt.has_value())
	{
		return;
	}

	handlerOpt.value()(command);
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

	//uint64_t iCurMS = Ctx::getCurMilliseconds();

	::login_msg::MSG_RESPONSE_ECHO response;
	response.set_value1(request.value1());
	response.set_value2(request.value2() + "|response");
	Network::OutputStream::sendMsgToUserByGateway(roleIdentifier, APie::OP_MSG_RESPONSE_ECHO, response);
}

}

