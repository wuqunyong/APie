#include "init.h"

#include "../rpc/client/rpc_client.h"
#include "../rpc/server/rpc_server.h"

namespace APie {
namespace RPC {

	void rpcInit()
	{
		RpcClientSingleton::get().init();
		RpcServerSingleton::get().init();
	}

} // namespace Network
} // namespace RPC
