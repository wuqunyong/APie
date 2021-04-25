#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <tuple>

#include "../../api/api.h"
#include "../../api/pb_handler.h"

#include "../../network/ctx.h"
#include "../../network/address.h"
#include "../../network/output_stream.h"
#include "../../network/windows_platform.h"
#include "../../singleton/threadsafe_singleton.h"

#include "../init.h"

#include <event2/util.h>


namespace APie {
namespace RPC {

	class RpcServer
	{
	public:
		bool init();

		using adaptor_type = std::function<std::tuple<uint32_t, std::string>(const ::rpc_msg::CLIENT_IDENTIFIER& client, ::google::protobuf::Message* ptrMsg)>;

		//template <typename Params>
		//using callback_type = std::function<std::tuple<uint32_t, std::string>(const ::rpc_msg::CLIENT_IDENTIFIER& client, const Params& params)>;

		//template <typename Params, typename std::enable_if<std::is_base_of<google::protobuf::Message, Params>::value, int>::type = 0 >
		//bool registerOpcodes(::rpc_msg::RPC_OPCODES opcodes, callback_type<Params> func)
		//{
		//	using OriginType = Params;
		//	std::string sType = OriginType::descriptor()->full_name();

		//	auto findIte = m_register.find(opcodes);
		//	if (findIte != m_register.end())
		//	{
		//		std::stringstream ss;
		//		ss << "duplicate opcode: " << opcodes;
		//		PANIC_ABORT(ss.str().c_str());

		//		return false;
		//	}

		//	auto ptrCb = [func](const ::rpc_msg::CLIENT_IDENTIFIER& client, ::google::protobuf::Message* ptrMsg) {
		//		OriginType *ptrData = dynamic_cast<OriginType*>(ptrMsg);
		//		if (nullptr == ptrData)
		//		{
		//			return std::make_tuple<uint32_t, std::string>(::rpc_msg::CODE_ParseError, "rpc_server|handleRequest");
		//		}

		//		return func(client, *ptrData);
		//	};


		//	m_register[opcodes] = ptrCb;
		//	m_types[opcodes] = sType;
		//	return true;
		//}

		template <typename F>
		bool bind(::rpc_msg::RPC_OPCODES opcodes, F func)
		{
			static_assert(Common::func_traits<F>::arg_count() == 2);

			using Args1Type = typename std::tuple_element<0, Common::func_traits<F>::args_type>::type;
			using Args2Type = typename std::tuple_element<1, Common::func_traits<F>::args_type>::type;

			static_assert(std::is_same<::rpc_msg::CLIENT_IDENTIFIER, Args1Type>::value);
			static_assert(std::is_base_of<google::protobuf::Message, Args2Type>::value);
			static_assert(std::is_same<std::tuple<uint32_t, std::string>, Common::func_traits<F>::result_type>::value);

			using OriginType = typename std::decay<Args2Type>::type;
			std::string sType = OriginType::descriptor()->full_name();

			auto findIte = m_register.find(opcodes);
			if (findIte != m_register.end())
			{
				std::stringstream ss;
				ss << "duplicate opcode: " << opcodes;
				PANIC_ABORT(ss.str().c_str());

				return false;
			}

			auto ptrCb = [func](const ::rpc_msg::CLIENT_IDENTIFIER& client, ::google::protobuf::Message* ptrMsg) {
				OriginType *ptrData = dynamic_cast<OriginType*>(ptrMsg);
				if (nullptr == ptrData)
				{
					return std::make_tuple<uint32_t, std::string>(::rpc_msg::CODE_ParseError, "rpc_server|handleRequest");
				}

				return func(client, *ptrData);
			};


			m_register[opcodes] = ptrCb;
			m_types[opcodes] = sType;
			return true;
		}

		bool asyncReply(const rpc_msg::CLIENT_IDENTIFIER& client, uint32_t errCode, const std::string& replyData);
		bool asyncStreamReply(const rpc_msg::CLIENT_IDENTIFIER& client, uint32_t errCode, const std::string& replyData, bool hasMore, uint32_t offset);

		static void handleRequest(uint64_t iSerialNum, ::rpc_msg::RPC_REQUEST& request);

	private:
		std::optional<std::string> getType(uint64_t opcode);
		std::optional<adaptor_type> findFunction(::rpc_msg::RPC_OPCODES opcodes);

	private:
		std::map<::rpc_msg::RPC_OPCODES, adaptor_type> m_register;
		std::map<uint64_t, std::string> m_types;
	};

	typedef APie::ThreadSafeSingleton<RpcServer> RpcServerSingleton;
} // namespace Network
} // namespace APie
