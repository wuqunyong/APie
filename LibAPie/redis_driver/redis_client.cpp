#include "redis_client.h"

#include "../network/command.h"
#include "../network/ctx.h"
#include "../network/logger.h"
#include "../rpc/client/rpc_client.h"
#include "../event/timer_impl.h"

namespace APie {

	RedisClient::RedisClient(Key key, const std::string &host, std::size_t port, const std::string &password, Cb cb) :
		m_key(key),
		m_host(host),
		m_port(port),
		m_password(password),
		m_cb(cb)
	{
		ASYNC_PIE_LOG("Redis/RedisClient", PIE_CYCLE_DAY, PIE_NOTICE, "ctor|key:%d-%d", (uint32_t)std::get<0>(key), std::get<1>(key));

		auto timerCb = [this]() {
			if (this->client().is_connected())
			{
				this->addReconnectTimer(3000);
				return;
			}

			std::stringstream ss;
			ss << "host:" << m_host << "|port:" << m_port << "|is_reconnecting:" << this->client().is_reconnecting();
			ASYNC_PIE_LOG("Redis/ReconnectTimer", PIE_CYCLE_DAY, PIE_WARNING, "%s", ss.str().c_str());

			if (this->getState() == RS_Disconnect)
			{
				try {
					auto ptrCb = this->getAdapterCb();
					m_client.connect(m_host, m_port, ptrCb);
					m_status = RS_Connect;
				}
				catch (std::exception& e) {
					std::stringstream ss;
					ss << "redis connect|Unexpected exception: " << e.what();

					PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "Exception", ss.str().c_str());
				}
			}

			this->addReconnectTimer(3000);
		};
		this->m_reconnectTimer = APie::CtxSingleton::get().getLogicThread()->dispatcher().createTimer(timerCb);
	}

	RedisClient::~RedisClient()
	{
		ASYNC_PIE_LOG("Redis/RedisClient", PIE_CYCLE_DAY, PIE_NOTICE, "destructor|key:%d-%d", (uint32_t)std::get<0>(m_key), std::get<1>(m_key));

		this->disableReconnectTimer();
	}

	void RedisClient::start()
	{
		if (m_started == 1)
		{
			return;
		}

		m_started = 1;

		auto sharedPtr = this->shared_from_this();

		auto ptrCb = [sharedPtr](const std::string &host, std::size_t port, cpp_redis::connect_state status) {
			
			std::stringstream ss;
			ss << "host:" << host << "|port:" << port << "|status:" << (uint32_t)status;
			ASYNC_PIE_LOG("Redis/Redis_ConnectCb", PIE_CYCLE_DAY, PIE_NOTICE, "%s", ss.str().c_str());

			sharedPtr->getCb()(host, port, status);

			switch (status)
			{
			case cpp_redis::connect_state::dropped:
				break;
			case cpp_redis::connect_state::start:
			{
				break;
			}
			case cpp_redis::connect_state::sleeping:
				break;
			case cpp_redis::connect_state::ok:
			{
				sharedPtr->setState(RS_Establish);

				if (sharedPtr->getPassword().empty())
				{
					return;
				}

				auto ptrAuth = [sharedPtr](cpp_redis::reply &reply) {
					if (reply.is_error())
					{
						sharedPtr->setAuth(3);

						std::stringstream ss;
						ss << "redis reply:" << reply.error();
						PIE_LOG("Redis/Redis_Auth", PIE_CYCLE_DAY, PIE_ERROR, "%s", ss.str().c_str());

						PANIC_ABORT(ss.str().c_str());
						return;
					}

					if (reply.is_bulk_string())
					{
						std::stringstream ss;
						ss << "redis reply:" << reply.as_string();
						ASYNC_PIE_LOG("Redis/Redis_Auth", PIE_CYCLE_DAY, PIE_NOTICE, "%s", ss.str().c_str());
					}
					sharedPtr->setAuth(2);
				};
				sharedPtr->client().auth(sharedPtr->getPassword(), ptrAuth);
				sharedPtr->client().commit();
				sharedPtr->setAuth(1);
				break;
			}
			case cpp_redis::connect_state::failed:
				break;
			case cpp_redis::connect_state::lookup_failed:
				break;
			case cpp_redis::connect_state::stopped:
			{
				sharedPtr->setState(RS_Disconnect);
				break;
			}
			default:
				break;
			}
		};

		m_adapterCb = ptrCb;
		m_client.connect(m_host, m_port, ptrCb);
		m_status = RS_Connect;
	}

	void RedisClient::addReconnectTimer(uint64_t interval)
	{
		this->m_reconnectTimer->enableTimer(std::chrono::milliseconds(interval));
	}

	void RedisClient::disableReconnectTimer()
	{
		this->m_reconnectTimer->disableTimer();
	}

	RedisClient::RedisStatus RedisClient::getState()
	{
		return m_status;
	}

	void RedisClient::setState(RedisStatus status)
	{
		m_status = status;
	}

	cpp_redis::client& RedisClient::client()
	{
		return m_client;
	}

	RedisClient::Cb& RedisClient::getCb()
	{
		return m_cb;
	}

	RedisClient::Cb& RedisClient::getAdapterCb()
	{
		return m_adapterCb;
	}

	std::string& RedisClient::getPassword()
	{
		return m_password;
	}

	RedisClient::Key RedisClient::getKey()
	{
		return m_key;
	}

	void RedisClient::setAuth(uint32_t value)
	{
		m_auth = value;
	}


	bool RedisClientFactory::registerClient(std::shared_ptr<RedisClient> ptrClient)
	{
		auto key = ptrClient->getKey();

		auto findIte = m_redisClient.find(key);
		if (findIte != m_redisClient.end())
		{
			return false;
		}

		m_redisClient[key] = ptrClient;
		ptrClient->start();
		ptrClient->addReconnectTimer(10000);

		return true;
	}

	std::shared_ptr<RedisClient> RedisClientFactory::getClient(RedisClient::Key key)
	{
		auto findIte = m_redisClient.find(key);
		if (findIte == m_redisClient.end())
		{
			return nullptr;
		}

		return findIte->second;
	}

	std::shared_ptr<RedisClient> RedisClientFactory::createClient(RedisClient::Key key, const std::string &host, std::size_t port, const std::string &password, RedisClient::Cb cb)
	{
		auto sharedPtr = std::make_shared<RedisClient>(key, host, port, password, cb);
		return sharedPtr;
	}

}