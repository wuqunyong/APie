#pragma once


#include <array>
#include <cstdint>
#include <string>

#include "../network/address.h"
#include "../network/windows_platform.h"

#include <event2/util.h>
#include <google/protobuf/message.h>



namespace APie {
namespace Network {
	enum class ConnetionType
	{
		CT_CLIENT = 0,
		CT_SERVER = 1,
	};

	class OutputStream
	{
	public:
		static void sendMsg(ConnetionType type, uint64_t iSerialNum, uint32_t iOpcode, const ::google::protobuf::Message& msg);
	};


} // namespace Network
} // namespace Envoy
