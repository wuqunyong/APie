#pragma once

#include <map>
#include <tuple> 
#include <optional>

#include <google/protobuf/message.h>

#include "../singleton/threadsafe_singleton.h"
#include "../network/logger.h"
#include "../common/utility.h"


namespace APie {
namespace Api {

class PBHandler {
public:
	using HandleFunction = std::function<void(uint64_t serialNum, ::google::protobuf::Message* ptrMsg)>;
	using HandleMuxFunction = std::function<void(uint64_t serialNum, uint32_t opcodes, const std::string& msg)>;

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

	//	auto ptrCb = [func](uint64_t serialNum, ::google::protobuf::Message* ptrMsg) {
	//		OriginType *ptrData = dynamic_cast<OriginType*>(ptrMsg);
	//		if (nullptr == ptrData)
	//		{
	//			return;
	//		}

	//		func(serialNum, *ptrData);
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

		static_assert(std::is_same<uint64_t, Args1Type>::value);
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
