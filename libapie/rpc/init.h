#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "../network/address.h"
#include "../network/windows_platform.h"

#include "../pb_msg.h"

#include <event2/util.h>

#define USE_NATS_PROXY true

namespace APie {
namespace RPC {

	void rpcInit();

} // namespace Network
} // namespace RPC
