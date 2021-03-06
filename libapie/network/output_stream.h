#pragma once


#include <array>
#include <cstdint>
#include <string>

#include "../network/address.h"
#include "../network/windows_platform.h"

#include "i_poll_events.hpp"

#include <event2/util.h>
#include <google/protobuf/message.h>

#include "../pb_msg.h"



namespace APie {
	class Command;

namespace Network {

	class OutputStream
	{
	public:
		static bool sendMsg(uint64_t iSerialNum, uint32_t iOpcode, const ::google::protobuf::Message& msg, ConnetionType type = ConnetionType::CT_NONE);
		static bool sendMsgByFlag(uint64_t iSerialNum, uint32_t iOpcode, const ::google::protobuf::Message& msg, uint32_t iFlag, ConnetionType type = ConnetionType::CT_NONE);
		static bool sendMsgByStr(uint64_t iSerialNum, uint32_t iOpcode, const std::string& msg, ConnetionType type = ConnetionType::CT_NONE);
		static bool sendMsgByStrByFlag(uint64_t iSerialNum, uint32_t iOpcode, const std::string& msg, uint32_t iFlag, ConnetionType type = ConnetionType::CT_NONE);
		
		static bool sendMsgToUserByGateway(const ::rpc_msg::RoleIdentifier& roleIdentifier, uint32_t iOpcode, const ::google::protobuf::Message& msg);


		static bool sendCommand(ConnetionType type, uint64_t iSerialNum, APie::Command& cmd);
	};


} // namespace Network
} // namespace Envoy
