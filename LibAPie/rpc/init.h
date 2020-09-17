#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "../network/address.h"
#include "../network/windows_platform.h"

#include "../../PBMsg/rpc_msg.pb.h"
#include "../../PBMsg/opcodes.pb.h"

#include <event2/util.h>


namespace APie {
namespace RPC {

	void rpcInit();

} // namespace Network
} // namespace RPC
