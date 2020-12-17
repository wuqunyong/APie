#pragma once

#include <map>
#include <tuple> 
#include <optional>

#include <google/protobuf/message.h>

#include "../singleton/threadsafe_singleton.h"
#include "../network/logger.h"


namespace APie {
namespace Api {

class PBHandler {
public:
	using HandleFunction = std::function<void(uint64_t serialNum, ::google::protobuf::Message* ptrMsg)>;
	using HandleMuxFunction = std::function<void(uint64_t serialNum, uint32_t opcodes, const std::string& msg)>;

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
			PANIC_ABORT(ss.str().c_str());

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

	static google::protobuf::Message* createMessage(const std::string& typeName);
	HandleMuxFunction& getDefaultFunc();
	void setDefaultFunc(HandleMuxFunction func);

private:
	std::map<uint64_t, HandleFunction> funcs_;
	std::map<uint64_t, std::string> types_;
	HandleMuxFunction default_func_;
};

class OpcodeHandler 
{
public:
	PBHandler client;
	PBHandler server;
};

typedef ThreadSafeSingleton<OpcodeHandler> OpcodeHandlerSingleton;

} // namespace Api
} // namespace Envoy
