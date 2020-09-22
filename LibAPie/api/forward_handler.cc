#include "../api/forward_handler.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <optional>

namespace APie {
namespace Api {

std::optional<PBForwardHandler::HandleFunction> PBForwardHandler::getFunction(uint64_t opcode)
{
	auto findIte = funcs_.find(opcode);
	if (findIte == funcs_.end())
	{
		return std::nullopt;
	}

	return std::optional<PBForwardHandler::HandleFunction>(findIte->second);
}

std::optional<std::string> PBForwardHandler::getType(uint64_t opcode)
{
	auto findIte = types_.find(opcode);
	if (findIte == types_.end())
	{
		return std::nullopt;
	}

	return std::optional<std::string>(findIte->second);
}

google::protobuf::Message* PBForwardHandler::createMessage(const std::string& typeName)
{
	google::protobuf::Message* message = NULL;
	const google::protobuf::Descriptor* descriptor = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(typeName);
	if (descriptor)
	{
		const google::protobuf::Message* prototype = google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
		if (prototype)
		{
			message = prototype->New();
		}
	}
	return message;
}

} // namespace Api
} // namespace Envoy
