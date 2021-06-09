#include "../api/pb_handler.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <optional>

namespace APie {
namespace Api {

std::optional<PBHandler::HandleFunction> PBHandler::getFunction(uint64_t opcode)
{
	auto findIte = funcs_.find(opcode);
	if (findIte == funcs_.end())
	{
		return std::nullopt;
	}

	return std::optional<PBHandler::HandleFunction>(findIte->second);
}

std::optional<std::string> PBHandler::getType(uint64_t opcode)
{
	auto findIte = types_.find(opcode);
	if (findIte == types_.end())
	{
		return std::nullopt;
	}

	return std::optional<std::string>(findIte->second);
}

google::protobuf::Message* PBHandler::createMessage(const std::string& typeName)
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

PBHandler::HandleMuxFunction& PBHandler::getDefaultFunc()
{
	return default_func_;
}

void PBHandler::setDefaultFunc(HandleMuxFunction func)
{
	default_func_ = func;
}

} // namespace Api
} // namespace Envoy
