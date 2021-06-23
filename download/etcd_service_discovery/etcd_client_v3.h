#pragma once

#include <memory>
#include <optional>
#include <ctime>
#include <chrono>

#include "etcd/Client.hpp"
#include "etcd/KeepAlive.hpp"
#include "etcd/Watcher.hpp"

#include "../../../msg/etcd_server_msg.pb.h"

#include "../network/logger.h"


namespace APie {
    struct WatchResponse
    {
        bool ok;
        std::string action;
        std::string key;
        std::string value;
    };


    struct ListResponse
    {
        bool ok;
        std::string errorMsg;
        std::vector<std::string> keys;
        std::vector<etcd::Value> values;
    };

    struct GetResponse
    {
        bool ok;
        std::string errorMsg;
        std::string value;
    };


class EtcdClientV3
{
public:
    EtcdClientV3();
    ~EtcdClientV3();

    void Init();

    std::optional<uint64_t> LeaseGrant(std::chrono::seconds seconds);
    bool Set(const std::string& key, const std::string& val, int64_t leaseId);
    GetResponse Get(const std::string& key);
    ListResponse List(const std::string& prefix);

    void DelKey();

    std::string GetPrefix();

private:
    bool Init_Impl(const std::string& endpoints, const std::string& prefix, const std::string& key);

    std::shared_ptr<etcd::KeepAlive> GetKeepAlive()
    {
        if (_keepalive.get() == nullptr)
        {
            _keepalive = _client->leasekeepalive(iExpiredTime).get();
        }
        return _keepalive;
    }

    void WatchForLeaderChange()
    {
        auto callback = [&](etcd::Response response) { this->WatchForLeaderChangeCallback(response); };
        _watcher = std::make_unique<etcd::Watcher>(*_client, _prefix, callback, true);
    }

    void WatchForLeaderChangeCallback(etcd::Response response);

private:
    bool _init = false;
    bool _addKey = false;

    int64_t _leaderId = 0;

    std::string _endpoint;
    std::string _prefix;
    std::string _key;

    etcd_msg::MSG_SERVER_REGISTER_INFO _registerInfo;

    std::shared_ptr<etcd::Client> _client;
    std::shared_ptr<etcd::KeepAlive> _keepalive;
    std::unique_ptr<etcd::Watcher> _watcher;
    std::function<void(WatchResponse)> _onWatch;

    static const uint32_t iExpiredTime = 60;
};


using EtcdSingleton = ThreadSafeSingleton<EtcdClientV3>;


} 

