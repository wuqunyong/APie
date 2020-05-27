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
	class OutputStream
	{
	public:
		static void sendMsg(uint64_t iSerialNum, uint32_t iOpcode, const ::google::protobuf::Message& msg);

	};


} // namespace Network
} // namespace Envoy
