#include "etcd_endpoint.h"



namespace APie {


    bool EtcdEndPoint::put(const std::string& key, const std::string& value)
    {
        std::lock_guard<std::mutex> guard(_sync);

        etcd_msg::MSG_SERVER_REGISTER_INFO info;
        if (!info.ParseFromString(value))
        {
            return false;
        }

        _keyInfo[key] = info;

        uint32_t serverType = info.server_type();
        uint32_t serverId = info.server_id();
        auto findIte = _type.find(serverType);
        if (findIte == _type.end())
        {
            std::vector<std::string> keySet;
            keySet.push_back(key);

            _type[info.server_type()] = keySet;
        }
        else
        {
            findIte->second.push_back(key);
        }

        auto tData = std::make_tuple(serverType, serverId);
        _typeKey[tData] = key;

        ASYNC_PIE_LOG("etcd", PIE_CYCLE_DAY, PIE_NOTICE, "etcd_endpoint|put|key:%s|value:%s", key.c_str(), info.ShortDebugString().c_str());

        return true;
    }

    void EtcdEndPoint::clear()
    {
        ASYNC_PIE_LOG("etcd", PIE_CYCLE_DAY, PIE_NOTICE, "etcd_endpoint|clear");

        std::lock_guard<std::mutex> guard(_sync);
        _type.clear();
        _typeKey.clear();
        _keyInfo.clear();
    }

    std::optional<::service_discovery::EndPointInstance> EtcdEndPoint::modulusEndpointById(uint32_t type, uint64_t matchId)
    {
        std::lock_guard<std::mutex> guard(_sync);

        auto findIte = _type.find(type);
        if (findIte == _type.end())
        {
            return std::nullopt;
        }

        if (findIte->second.empty())
        {
            return std::nullopt;
        }

        uint32_t iSize = findIte->second.size();

        uint32_t iIndex = matchId % iSize;
        std::string key = findIte->second[iIndex];

        auto ite = _keyInfo.find(key);
        if (ite == _keyInfo.end())
        {
            return std::nullopt;
        }

        uint32_t iType = ite->second.server_type();
        if (!common::EndPointType_IsValid(iType))
        {
            return std::nullopt;
        }

        ::service_discovery::EndPointInstance instance;
        instance.set_type((common::EndPointType)ite->second.server_type());
        instance.set_id(ite->second.server_id());

        if ((common::EndPointType)ite->second.server_type() == common::EPT_Gateway_Server)
        {
            instance.set_ip(ite->second.gw_out_ip());
            instance.set_port(ite->second.gw_out_port());
        }

        return instance;
    }
}
