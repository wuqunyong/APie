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
#include <utility>

#include <cpp_redis/cpp_redis>

#include "../singleton/threadsafe_singleton.h"
#include "../../pb_msg/core/rpc_msg.pb.h"
#include "../event/timer.h"



namespace APie {

	enum REDIS_CLIENT_TYPE
	{
		RCT_None = 0,
		RCT_Role = 1,
	};

	template<typename... Args, std::size_t... I>
	std::string _GenRedisKeyImpl(std::tuple<Args...> &&params, std::index_sequence<I...>)
	{
		std::stringstream ss;
		((ss << (I == 0 ? "" : ":") << std::get<I>(params)), ...);

		return ss.str();
	}

	template<typename... Args>
	std::string GenRedisKey(Args... args)
	{
		return _GenRedisKeyImpl(std::forward<std::tuple<Args...>>(std::make_tuple(args...)), std::index_sequence_for<Args...>{});
	}

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
			RA_Ok = 2,    // 认证成功
			RA_Error = 3, // 认证失败
		};

		using Key = std::tuple<uint32_t, uint32_t>;
		using Cb = std::function<void(std::shared_ptr<RedisClient> ptrClient)>;
		using AdapterCb = std::function<void(const std::string &host, std::size_t port, cpp_redis::connect_state status)>;

		RedisClient(Key key, const std::string &host, std::size_t port, const std::string &password, Cb cb);
		~RedisClient();

		void start();
		void stop();
		
		cpp_redis::client& client();

		Cb& getCb();
		AdapterCb& getAdapterCb();

		std::string& getPassword();
		Key getKey();
		
		
		void setAuth(RedisAuth value);
		void addReconnectTimer(uint64_t interval);
		void disableReconnectTimer();

		RedisStatus getState();
		void setState(RedisStatus status);

		friend std::ostream& operator<<(std::ostream& out, const RedisClient& client) {
			out  << "{" << client.m_host << std::string(":") << client.m_port << std::string(":") << client.m_password 
				<< std::string(":") << client.m_status << std::string(":") << client.m_auth << "}";
			return out;
		}

	private:
		Key m_key;
		std::string m_host;
		std::size_t m_port;
		std::string m_password;

		Cb m_cb;
		AdapterCb m_adapterCb;
		
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
		void destroy();

		std::shared_ptr<RedisClient> getClient(RedisClient::Key key);
		std::shared_ptr<RedisClient> getConnectedClient(RedisClient::Key key);

	public:
		static std::shared_ptr<RedisClient> createClient(RedisClient::Key key, const std::string &host, std::size_t port, const std::string &password, RedisClient::Cb cb);

	private:
		std::map<RedisClient::Key, std::shared_ptr<RedisClient>> m_redisClient;
	};

	using RedisClientFactorySingleton = ThreadSafeSingleton<RedisClientFactory>;
}
