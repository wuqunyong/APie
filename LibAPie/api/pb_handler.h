#pragma once

#include <map>
#include <tuple> 
#include <optional>

#include <google/protobuf/message.h>

#include "../singleton/threadsafe_singleton.h"


namespace Envoy {
namespace Api {

using PBCb = std::function<void(uint64_t serialNum, std::shared_ptr<::google::protobuf::Message> ptrMsg)>;

class PBHandler {
public:
  bool registerHandler(uint32_t opcode, const ::google::protobuf::Message& msg, PBCb cb);
  std::optional<std::tuple<std::shared_ptr<::google::protobuf::Message>, PBCb>> get(uint32_t opcode);


private:
	std::map<uint32_t, std::tuple<std::shared_ptr<::google::protobuf::Message>, PBCb>> handler_;
};

typedef ThreadSafeSingleton<PBHandler> PBHandlerSingleton;

} // namespace Api
} // namespace Envoy
