#include "rpc_server.h"


namespace APie {
namespace RPC {

	bool RpcServer::init()
	{
		APie::Api::OpcodeHandlerSingleton::get().client.bind(::opcodes::OPCODE_ID::OP_RPC_REQUEST, RpcServer::handleRequest);
		APie::Api::OpcodeHandlerSingleton::get().server.bind(::opcodes::OPCODE_ID::OP_RPC_REQUEST, RpcServer::handleRequest);
		return true;
	}

	bool RpcServer::asyncReply(const rpc_msg::CLIENT_IDENTIFIER& client, uint32_t errCode, const std::string& replyData)
	{
		uint64_t iSerialNum = client.channel_serial_num();

		::rpc_msg::CHANNEL server;
		server.set_type(APie::CtxSingleton::get().identify().type);
		server.set_type(APie::CtxSingleton::get().identify().id);

		::rpc_msg::RPC_RESPONSE response;
		*response.mutable_client() = client;
		*response.mutable_server()->mutable_stub() = server;
		response.mutable_status()->set_code(errCode);
		response.set_result_data(replyData);

#ifdef USE_NATS_PROXY
		std::string channel = std::to_string(client.stub().type()) + ":" + std::to_string(client.stub().id());

		::nats_msg::NATS_MSG_PRXOY nats_msg;
		(*nats_msg.mutable_rpc_response()) = response;
		APie::Event::NatsSingleton::get().publish(channel, nats_msg);
#else
		bool bResult = APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_RPC_RESPONSE, response);
		if (!bResult)
		{
			//TODO
		}
#endif
		return true;
	}

	bool RpcServer::asyncStreamReply(const rpc_msg::CLIENT_IDENTIFIER& client, uint32_t errCode, const std::string& replyData, bool hasMore, uint32_t offset)
	{
		uint64_t iSerialNum = client.channel_serial_num();

		::rpc_msg::CHANNEL server;
		server.set_type(APie::CtxSingleton::get().identify().type);
		server.set_type(APie::CtxSingleton::get().identify().id);

		::rpc_msg::RPC_RESPONSE response;
		*response.mutable_client() = client;
		*response.mutable_server()->mutable_stub() = server;
		response.mutable_status()->set_code(errCode);
		response.set_result_data(replyData);
		response.set_has_more(hasMore);
		response.set_offset(offset);

#ifdef USE_NATS_PROXY
		std::string channel = std::to_string(client.stub().type()) + ":" + std::to_string(client.stub().id());

		::nats_msg::NATS_MSG_PRXOY nats_msg;
		(*nats_msg.mutable_rpc_response()) = response;
		APie::Event::NatsSingleton::get().publish(channel, nats_msg);
#else
		bool bResult = APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_RPC_RESPONSE, response);
		if (!bResult)
		{
			//TODO
		}
#endif
		return true;
	}

	void RpcServer::handleRequest(uint64_t iSerialNum, ::rpc_msg::RPC_REQUEST& request)
	{
		::rpc_msg::CHANNEL server;
		server.set_type(APie::CtxSingleton::get().identify().type);
		server.set_id(APie::CtxSingleton::get().identify().id);

		request.mutable_client()->set_channel_serial_num(iSerialNum);

		::rpc_msg::RPC_RESPONSE response;
		*response.mutable_client() = request.client();
		*response.mutable_server()->mutable_stub() = server;

		if (request.server().stub().type() != server.type() || request.server().stub().id() != server.id())
		{
			if (server.type() != common::EPT_Route_Proxy)
			{
				response.mutable_status()->set_code(::rpc_msg::RPC_CODE::CODE_ErrorServerPost);
				APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_RPC_RESPONSE, response);
				return;
			}
			else
			{
				EndPoint sendTarget;
				sendTarget.type = request.server().stub().type();
				sendTarget.id = request.server().stub().id();
				auto serialOpt = EndPointMgrSingleton::get().getSerialNum(sendTarget);
				if (!serialOpt.has_value())
				{
					response.mutable_status()->set_code(::rpc_msg::RPC_CODE::CODE_RouteNotLinkToServer);
					APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_RPC_RESPONSE, response);
					return;
				}

				request.mutable_client()->mutable_router()->set_type(server.type());
				request.mutable_client()->mutable_router()->set_id(server.id());
				bool bResult = APie::Network::OutputStream::sendMsg(serialOpt.value(), ::opcodes::OPCODE_ID::OP_RPC_REQUEST, request);
				if (!bResult)
				{
					response.mutable_status()->set_code(::rpc_msg::RPC_CODE::CODE_RouteSendToServerError);
					APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_RPC_RESPONSE, response);
				}
				return;
			}
		}

		auto functionOpt = RpcServerSingleton::get().findFunction(request.opcodes());
		if (!functionOpt.has_value())
		{
			response.mutable_status()->set_code(::rpc_msg::RPC_CODE::CODE_Unregister);
			APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_RPC_RESPONSE, response);
			return;
		}

		auto typeOpt = RpcServerSingleton::get().getType(request.opcodes());
		if (!typeOpt.has_value())
		{
			response.mutable_status()->set_code(::rpc_msg::RPC_CODE::CODE_Unregister);
			APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_RPC_RESPONSE, response);
			return;
		}

		std::string sType = typeOpt.value();
		auto ptrMsg = Api::PBHandler::createMessage(sType);
		if (ptrMsg == nullptr)
		{
			return;
		}

		std::shared_ptr<::google::protobuf::Message> newMsg(ptrMsg);
		bool bResult = newMsg->ParseFromString(request.args_data());
		if (!bResult)
		{
			return;
		}

		auto tupleResult = functionOpt.value()(request.client(), newMsg.get());

		uint32_t iCode = std::get<0>(tupleResult);
		if (iCode == ::rpc_msg::RPC_CODE::CODE_Ok_Async)
		{
			return;
		}

		if (!request.client().required_reply())
		{
			return;
		}

		response.mutable_status()->set_code(iCode);
		response.set_result_data(std::get<1>(tupleResult));
		bResult = APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_RPC_RESPONSE, response);
		if (!bResult)
		{
			//TODO
		}
		return;
	}

	std::optional<std::string> RpcServer::getType(uint64_t opcode)
	{
		auto findIte = m_types.find(opcode);
		if (findIte == m_types.end())
		{
			return std::nullopt;
		}

		return findIte->second;
	}

	std::optional<RpcServer::adaptor_type> RpcServer::findFunction(::rpc_msg::RPC_OPCODES opcodes)
	{
		auto findIte = m_register.find(opcodes);
		if (findIte == m_register.end())
		{
			return std::nullopt;
		}

		return findIte->second;
	}

} // namespace RPC
} // namespace APie
