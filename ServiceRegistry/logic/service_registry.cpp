#include "service_registry.h"

#include "apie.h"

namespace APie {

void ServiceRegistry::init()
{
	APie::Api::OpcodeHandlerSingleton::get().server.bind(::opcodes::OPCODE_ID::OP_MSG_REQUEST_ADD_INSTANCE, ServiceRegistry::handleRequestAddInstance, ::service_discovery::MSG_REQUEST_ADD_INSTANCE::default_instance());
}

void ServiceRegistry::handleRequestAddInstance(uint64_t iSerialNum, ::service_discovery::MSG_REQUEST_ADD_INSTANCE request)
{
	::service_discovery::MSG_RESP_ADD_INSTANCE response;
	response.set_status_code(::opcodes::StatusCode::SC_Ok);

	APie::Network::OutputStream::sendMsg(iSerialNum, ::opcodes::OPCODE_ID::OP_MSG_RESP_ADD_INSTANCE, response);
}

}

