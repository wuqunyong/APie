#pragma once


#include <array>
#include <cstdint>
#include <string>

#include "../network/address.h"
#include "../network/windows_platform.h"

#include "i_poll_events.hpp"

#include <event2/util.h>
#include <google/protobuf/message.h>



namespace APie {
namespace Network {

	class OutputStream
	{
	public:
		static bool sendMsg(uint64_t iSerialNum, uint32_t iOpcode, const ::google::protobuf::Message& msg, ConnetionType type = ConnetionType::CT_NONE);
	};


} // namespace Network
} // namespace Envoy
