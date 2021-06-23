#include "etcd_client_v3.h"

#include "etcd_endpoint.h"

#include "../event/nats_proxy.h"

namespace APie {

EtcdClientV3::EtcdClientV3()
{
}

EtcdClientV3::~EtcdClientV3()
{
    this->DelKey();

    if (_watcher.get() != nullptr)
    {
        _watcher->Cancel();
    }

    if (_keepalive.get() != nullptr)
    {
        _keepalive->Cancel();
    }
}

void EtcdClientV3::Init()
{
    bool enable = APie::CtxSingleton::get().yamlAs<bool>({ "etcd","enable" }, false);
    if (!enable)
    {
        return;
    }


    std::string endpoints = APie::CtxSingleton::get().yamlAs<std::string>({ "etcd","urls" }, "");

    uint32_t id = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","id" }, 0);
    uint32_t type = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","type" }, 0);
    std::string channel = APie::Event::NatsManager::GetTopicChannel(type, id);

    auto sSub = APie::CtxSingleton::get().yamlAs<std::string>({ "nats", "channel_domains" }, "");

    std::string key = sSub + "/" + channel;

    bool bResult = Init_Impl(endpoints, sSub, key);
    if (!bResult)
    {
        PANIC_ABORT("etcd init error");
    }
    PIE_LOG("startup/startup", PIE_CYCLE_HOUR, PIE_NOTICE, "etcd|%s|%s|%s", endpoints.c_str(), sSub.c_str(), key.c_str());
}

bool EtcdClientV3::Init_Impl(const std::string& endpoints, const std::string& prefix, const std::string& key)
{
    if (_init)
    {
        return false;
    }
    _init = true;

    _endpoint = endpoints;
    _prefix = prefix;
    _key = key;

    _client = std::make_shared<etcd::Client>(_endpoint);

    //std::chrono::seconds iSecond(iExpiredTime);
    //auto optValue = this->LeaseGrant(iSecond);
    //if (!optValue.has_value())
    //{
    //    return false;
    //}

    //_leaderId = optValue.value();

    auto iCurPoint = std::chrono::system_clock::now();

    _registerInfo.set_start_time(iCurPoint.time_since_epoch().count());
    _registerInfo.set_status(etcd_msg::E_SERVER_STATUS_READY);
    
    auto serverType = APie::CtxSingleton::get().getServerType();
    auto sereverId = APie::CtxSingleton::get().getServerId();
    _registerInfo.set_server_type(serverType);
    _registerInfo.set_server_id(sereverId);

    if (serverType == common::EPT_Gateway_Server)
    {
        std::string ip = APie::CtxSingleton::get().yamlAs<std::string>({ "identify","out_ip" }, "");
        uint16_t port = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","out_port" }, 0);

        _registerInfo.set_gw_out_ip(ip);
        _registerInfo.set_gw_out_port(port);
    }


    _leaderId = GetKeepAlive()->Lease();
    etcd::Response addRes = _client->add(_key, _registerInfo.SerializeAsString(), _leaderId).get();
    if (!addRes.is_ok())
    {
        ASYNC_PIE_LOG("etcd", PIE_CYCLE_DAY, PIE_ERROR, "etcd|add failure|key:%s|error:%s", _key.c_str(), addRes.error_message().c_str());
        return false;
    }
    else
    {
        _addKey = true;
        ASYNC_PIE_LOG("etcd", PIE_CYCLE_DAY, PIE_NOTICE, "etcd|add success|key:%s", _key.c_str());
    }

    auto resp = this->List(_prefix);
    for (auto& elems : resp.values)
    {
        EtcdEndPointSingleton::get().put(elems.key(), elems.as_string());
    }

    this->WatchForLeaderChange();

    return true;
}

std::optional<uint64_t> EtcdClientV3::LeaseGrant(std::chrono::seconds seconds)
{
    etcd::Response etcdRes = _client->leasegrant(seconds.count()).get();

    if (!etcdRes.is_ok()) 
    {
        ASYNC_PIE_LOG("etcd", PIE_CYCLE_DAY, PIE_ERROR, "leasegrant|%s", etcdRes.error_message().c_str());
        return std::nullopt;
    }

    return etcdRes.value().lease();
}


bool EtcdClientV3::Set(const std::string& key, const std::string& val, int64_t leaseId)
{
    etcd::Response etcdRes = _client->set(key, val, leaseId).get();
    if (!etcdRes.is_ok()) 
    {
        ASYNC_PIE_LOG("etcd", PIE_CYCLE_DAY, PIE_ERROR, "etcd|set|%s", etcdRes.error_message().c_str());
        return false;
    }
    return true;
}

GetResponse
EtcdClientV3::Get(const std::string& key)
{
    etcd::Response etcdRes = _client->get(key).get();

    GetResponse res;
    res.ok = etcdRes.is_ok();
    res.value = etcdRes.value().as_string();
    if (!res.ok)
    {
        ASYNC_PIE_LOG("etcd", PIE_CYCLE_DAY, PIE_ERROR, "etcd|get|%s", etcdRes.error_message().c_str());
    }
    return res;
}

ListResponse
EtcdClientV3::List(const std::string& prefix)
{
    etcd::Response etcdRes = _client->ls(prefix).get();

    ListResponse res;
    res.ok = etcdRes.is_ok();
    res.keys = std::move(etcdRes.keys());
    res.values = std::move(etcdRes.values());
    if (!res.ok) 
    {
        ASYNC_PIE_LOG("etcd", PIE_CYCLE_DAY, PIE_ERROR, "etcd|ls|%s", etcdRes.error_message().c_str());
    }
    return res;
}

void EtcdClientV3::DelKey()
{
    if (!_addKey)
    {
        return;
    }

    etcd::Response res = _client->rm(_key).get();
    if (!res.is_ok())
    {
        ASYNC_PIE_LOG("etcd", PIE_CYCLE_DAY, PIE_ERROR, "etcd|del error|key:%s|error:%s", _key.c_str(), res.error_message().c_str());
    }
    else
    {
        _addKey = false;
        ASYNC_PIE_LOG("etcd", PIE_CYCLE_DAY, PIE_NOTICE, "etcd|del success|key:%s", _key.c_str());
    }
}

std::string EtcdClientV3::GetPrefix()
{
    return _prefix;
}

void EtcdClientV3::WatchForLeaderChangeCallback(etcd::Response response)
{
    ASYNC_PIE_LOG("etcd", PIE_CYCLE_DAY, PIE_NOTICE, "etcd|watchChange|action:%s|key:%s", response.action().c_str(), response.value().key().c_str());

    ListResponse resp = this->List(_prefix);
    if (!resp.ok)
    {
        return;
    }

    EtcdEndPointSingleton::get().clear();

    for (auto& elems : resp.values)
    {
        EtcdEndPointSingleton::get().put(elems.key(), elems.as_string());
    }
}

} 
