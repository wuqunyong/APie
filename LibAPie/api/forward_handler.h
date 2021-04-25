#pragma once

#include <map>
#include <tuple> 
#include <optional>

#include <google/protobuf/message.h>

#include "../singleton/threadsafe_singleton.h"
#include "../network/logger.h"
#include "../common/utility.h"

#include "../../PBMsg/rpc_msg.pb.h"

namespace APie {
namespace Api {

class PBForwardHandler {
public:
	using HandleFunction = std::function<void(::rpc_msg::RoleIdentifier roleIdentifier, ::google::protobuf::Message* ptrMsg)>;

	//template <typename F, typename Params, typename std::enable_if<
	//	std::is_base_of<google::protobuf::Message, Params>::value, int>::type = 0 >
	//	bool bind(uint64_t opcode, F func, Params params)
	//{
	//	using OriginType = Params;
	//	std::string sType = OriginType::descriptor()->full_name();

	//	auto findIte = funcs_.find(opcode);
	//	if (findIte != funcs_.end())
	//	{
	//		std::stringstream ss;
	//		ss << "duplicate opcode: " << opcode;
	//		PANIC_ABORT(ss.str().c_str());

	//		return false;
	//	}

	//	auto ptrCb = [func](::rpc_msg::RoleIdentifier roleIdentifier, ::google::protobuf::Message* ptrMsg) {
	//		OriginType *ptrData = dynamic_cast<OriginType*>(ptrMsg);
	//		if (nullptr == ptrData)
	//		{
	//			return;
	//		}

	//		func(roleIdentifier, *ptrData);
	//	};


	//	funcs_[opcode] = ptrCb;
	//	types_[opcode] = sType;
	//	
	//	return true;
	//}

	template <typename F>
	bool bind(uint64_t opcode, F func)
	{
		static_assert(Common::func_traits<F>::arg_count() == 2);

		using Args1Type = typename std::tuple_element<0, Common::func_traits<F>::args_type>::type;
		using Args2Type = typename std::tuple_element<1, Common::func_traits<F>::args_type>::type;

		static_assert(std::is_same<::rpc_msg::RoleIdentifier, Args1Type>::value);
		static_assert(std::is_base_of<google::protobuf::Message, Args2Type>::value);

		using OriginType = typename std::decay<Args2Type>::type;
		std::string sType = OriginType::descriptor()->full_name();

		auto findIte = funcs_.find(opcode);
		if (findIte != funcs_.end())
		{
			std::stringstream ss;
			ss << "duplicate opcode: " << opcode;
			PANIC_ABORT(ss.str().c_str());

			return false;
		}

		auto ptrCb = [func](::rpc_msg::RoleIdentifier roleIdentifier, ::google::protobuf::Message* ptrMsg) {
			OriginType *ptrData = dynamic_cast<OriginType*>(ptrMsg);
			if (nullptr == ptrData)
			{
				return;
			}

			func(roleIdentifier, *ptrData);
		};


		funcs_[opcode] = ptrCb;
		types_[opcode] = sType;

		return true;
	}

	std::optional<std::string> getType(uint64_t opcode);
	std::optional<HandleFunction> getFunction(uint64_t opcode);

	google::protobuf::Message* createMessage(const std::string& typeName);

private:
	std::map<uint64_t, HandleFunction> funcs_;
	std::map<uint64_t, std::string> types_;
};

class ForwardHandler
{
public:
	//PBForwardHandler client;
	PBForwardHandler server;
};

typedef ThreadSafeSingleton<ForwardHandler> ForwardHandlerSingleton;


} // namespace Api
} // namespace Envoy
