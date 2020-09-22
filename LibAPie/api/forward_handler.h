#pragma once

#include <map>
#include <tuple> 
#include <optional>

#include <google/protobuf/message.h>

#include "../singleton/threadsafe_singleton.h"
#include "../network/logger.h"

#include "../../PBMsg/rpc_msg.pb.h"

namespace APie {
namespace Api {

class PBForwardHandler {
public:
	using HandleFunction = std::function<void(::rpc_msg::RoleIdentifier roleIdentifier, ::google::protobuf::Message* ptrMsg)>;

	template <typename F, typename Params, typename std::enable_if<
		std::is_base_of<google::protobuf::Message, Params>::value, int>::type = 0 >
		bool bind(uint64_t opcode, F func, Params params)
	{
		using OriginType = Params;
		std::string sType = OriginType::descriptor()->full_name();

		auto findIte = funcs_.find(opcode);
		if (findIte != funcs_.end())
		{
			std::stringstream ss;
			ss << "duplicate opcode: " << opcode;
			fatalExit(ss.str().c_str());

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
	PBForwardHandler client;
	PBForwardHandler server;
};

typedef ThreadSafeSingleton<ForwardHandler> ForwardHandlerSingleton;

} // namespace Api
} // namespace Envoy
