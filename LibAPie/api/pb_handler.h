#pragma once

#include <map>
#include <tuple> 
#include <optional>

#include <google/protobuf/message.h>

#include "../singleton/threadsafe_singleton.h"


namespace APie {
namespace Api {

class PBHandler {
public:
	using HandleFunction = std::function<void(uint64_t serialNum, ::google::protobuf::Message* ptrMsg)>;


	template <typename F, typename Params, typename std::enable_if<
		std::is_base_of<google::protobuf::Message, Params>::value, int>::type = 0 >
		bool bind(uint64_t opcode, F func, Params params)
	{
		using OriginType = Params;
		std::string sType = OriginType::descriptor()->full_name();

		auto findIte = funcs_.find(opcode);
		if (findIte != funcs_.end())
		{
			return false;
		}

		auto ptrCb = [func](uint64_t serialNum, ::google::protobuf::Message* ptrMsg) {
			OriginType *ptrData = dynamic_cast<OriginType*>(ptrMsg);
			if (nullptr == ptrData)
			{
				return;
			}

			func(serialNum, *ptrData);
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


typedef ThreadSafeSingleton<PBHandler> PBHandlerSingleton;

} // namespace Api
} // namespace Envoy
