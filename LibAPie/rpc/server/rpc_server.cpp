#include "rpc_server.h"


namespace APie {
namespace RPC {

	bool RpcServer::init()
	{
		APie::Api::OpcodeHandlerSingleton::get().server.bind(::opcodes::OPCODE_ID::OP_RPC_REQUEST, RpcServer::handleRequest, ::rpc_msg::RPC_REQUEST::default_instance());
		return true;
	}

	bool RpcServer::registerOpcodes(::rpc_msg::RPC_OPCODES opcodes, RpcServerCb cb)
	{
		auto findIte = m_register.find(opcodes);
		if (findIte != m_register.end())
		{
			return false;
		}

		m_register[opcodes] = cb;
		return true;
	}

	bool RpcServer::asyncReply(uint64_t iSerialNum, const rpc_msg::CLIENT_IDENTIFIER& client, uint32_t errCode, const std::string& replyData)
	{
		::rpc_msg::CHANNEL server;
		server.set_type(APie::CtxSingleton::get().identify().type);
		server.set_type(APie::CtxSingleton::get().identify().id);

		::rpc_msg::RPC_RESPONSE response;
		*response.mutable_client() = client;
		*response.mutable_server()->mutable_stub() = server;
		response.mutable_status()->set_code(errCode);
		response.set_result_data(replyData);

		bool bResult = APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_RPC_RESPONSE, response);
		if (!bResult)
		{
			//TODO
		}
		return true;
	}

	void RpcServer::handleRequest(uint64_t iSerialNum, ::rpc_msg::RPC_REQUEST request)
	{
		::rpc_msg::CHANNEL server;
		server.set_type(APie::CtxSingleton::get().identify().type);
		server.set_id(APie::CtxSingleton::get().identify().id);

		::rpc_msg::RPC_RESPONSE response;
		*response.mutable_client() = request.client();
		*response.mutable_server()->mutable_stub() = server;

		auto cb = RpcServerSingleton::get().find(request.opcodes());
		if (cb == nullptr)
		{
			response.mutable_status()->set_code(::rpc_msg::RPC_CODE::CODE_Unregister);
			APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_RPC_RESPONSE, response);
			return;
		}

		auto tupleResult = cb(request.client(), request.args_data());

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
		bool bResult = APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_RPC_RESPONSE, response);
		if (!bResult)
		{
			//TODO
		}
		return;
	}

	RpcServerCb RpcServer::find(::rpc_msg::RPC_OPCODES opcodes)
	{
		auto findIte = m_register.find(opcodes);
		if (findIte == m_register.end())
		{
			return nullptr;
		}

		return findIte->second;
	}

} // namespace RPC
} // namespace APie
