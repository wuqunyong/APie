#include "../api/pb_handler.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <optional>

namespace Envoy {
namespace Api {

bool PBHandler::registerHandler(uint32_t opcode, const ::google::protobuf::Message& msg, PBCb cb)
{
	auto findIte = handler_.find(opcode);
	if (findIte != handler_.end())
	{
		return false;
	}

	std::shared_ptr<::google::protobuf::Message> ptrMsg(msg.New());
	ptrMsg->CopyFrom(msg);
	handler_[opcode] = std::tuple<std::shared_ptr<::google::protobuf::Message>, PBCb>(ptrMsg, cb);
	return true;
}

std::optional<std::tuple<std::shared_ptr<::google::protobuf::Message>, PBCb>> PBHandler::get(uint32_t opcode)
{
	auto findIte = handler_.find(opcode);
	if (findIte == handler_.end())
	{
		return std::nullopt;
	}

	return std::optional<std::tuple<std::shared_ptr<::google::protobuf::Message>, PBCb>>(findIte->second);
}

} // namespace Api
} // namespace Envoy
