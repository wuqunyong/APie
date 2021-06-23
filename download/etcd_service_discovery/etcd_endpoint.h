#pragma once

#include <memory>
#include <optional>
#include <ctime>
#include <chrono>
#include <tuple>
#include <vector>
#include <optional>

#include "../../../msg/etcd_server_msg.pb.h"
#include "../../../msg/service_discovery.pb.h"

#include "../network/logger.h"


namespace APie {


class EtcdEndPoint
{
public:

    bool put(const std::string& key, const std::string& value);
    void clear();

    std::optional<::service_discovery::EndPointInstance> modulusEndpointById(uint32_t type, uint64_t matchId);

private:
    std::mutex _sync;
    std::map<uint32_t, std::vector<std::string>> _type;
    std::map<std::tuple<uint32_t, uint32_t>, std::string> _typeKey;
    std::map<std::string, etcd_msg::MSG_SERVER_REGISTER_INFO> _keyInfo;
};


using EtcdEndPointSingleton = ThreadSafeSingleton<EtcdEndPoint>;


} 

