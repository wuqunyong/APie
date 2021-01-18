#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>
#include <memory>
#include <functional>
#include <optional>

#include <cpp_redis/cpp_redis>

#include "../singleton/threadsafe_singleton.h"
#include "../../PBMsg/rpc_msg.pb.h"
#include "../event/timer.h"


namespace APie {

	enum REDIS_CLIENT_TYPE
	{
		RCT_None = 0,
		RCT_Role = 1,
	};

	class RedisClient : public std::enable_shared_from_this<RedisClient>
	{
	public:
		enum RedisStatus
		{
			RS_None = 0,
			RS_Connect = 1,
			RS_Auth = 2,
			RS_Established = 3,
			RS_Closed = 4,
		};

		enum RedisAuth
		{
			RA_None = 0,  // 
			RA_Doing = 1, // 进行认证
			RA_Ok = 2,    // 认证错误
			RA_Error = 3, // 认证成功
		};

		using Key = std::tuple<uint32_t, uint32_t>;
		using Cb = std::function<void(const std::string &host, std::size_t port, cpp_redis::connect_state status)>;

		RedisClient(Key key, const std::string &host, std::size_t port, const std::string &password, Cb cb);
		~RedisClient();

		void start();
		
		cpp_redis::client& client();

		Cb& getCb();
		Cb& getAdapterCb();

		std::string& getPassword();
		Key getKey();
		
		
		void setAuth(RedisAuth value);
		void addReconnectTimer(uint64_t interval);
		void disableReconnectTimer();

		RedisStatus getState();
		void setState(RedisStatus status);

	private:
		Key m_key;
		std::string m_host;
		std::size_t m_port;
		std::string m_password;
		Cb m_cb;
		Cb m_adapterCb;
		
		uint32_t m_started = 0;
		RedisAuth m_auth = RA_None;
		RedisStatus m_status = RS_None;

		cpp_redis::client m_client;
		Event::TimerPtr m_reconnectTimer;
	};


	class RedisClientFactory
	{
	public:
		bool registerClient(std::shared_ptr<RedisClient> ptrClient);
		std::shared_ptr<RedisClient> getClient(RedisClient::Key key);

	public:
		static std::shared_ptr<RedisClient> createClient(RedisClient::Key key, const std::string &host, std::size_t port, const std::string &password, RedisClient::Cb cb);

	private:
		std::map<RedisClient::Key, std::shared_ptr<RedisClient>> m_redisClient;
	};

	using RedisClientFactorySingleton = ThreadSafeSingleton<RedisClientFactory>;
}
