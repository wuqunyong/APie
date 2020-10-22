#pragma once

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <atomic>

#include "../singleton/threadsafe_singleton.h"
#include "../event/dispatched_thread.h"
#include "../network/platform_impl.h"
#include "../network/i_poll_events.hpp"
#include "../network/end_point.h"

#include "../../PBMsg/rpc_msg.pb.h"

#include "yaml-cpp/yaml.h"

namespace APie
{
    //  Context object encapsulates all the global state associated with
    //  the library.

    class Ctx
    {
    public:
		Ctx();
		~Ctx();

		EndPoint identify();
		std::shared_ptr<SelfRegistration> getEndpoint();
		uint64_t getNowMilliseconds();
		uint64_t getNowSeconds();

		uint32_t generateHash(EndPoint point);

		void init(const std::string& configFile);
		void start();
		void destroy();

		void waitForShutdown();

		uint32_t generatorTId();
		YAML::Node& yamlNode();

		std::shared_ptr<Event::DispatchedThreadImpl> chooseIOThread();
		std::shared_ptr<Event::DispatchedThreadImpl> getLogicThread();
		std::shared_ptr<Event::DispatchedThreadImpl> getLogThread();
		std::shared_ptr<Event::DispatchedThreadImpl> getMetricsThread();
		std::shared_ptr<Event::DispatchedThreadImpl> getDBThread();

		std::shared_ptr<Event::DispatchedThreadImpl> getThreadById(uint32_t id);

		template <typename T>
		T yamlAs(std::vector<std::string> index)
		{
			std::lock_guard<std::mutex> guard(node_sync_);

			//YAML::Node node = YAML::Clone(node_);
			////YAML::Node node = node_;
			//for (const auto &items : index)
			//{
			//	if (node[items])
			//	{
			//		node = node[items];
			//	}
			//	else
			//	{
			//		throw std::invalid_argument("Configuration|field:" + items + "|not found");
			//	}
			//}

			std::vector<YAML::Node> nodeList;
			nodeList.push_back(node_);

			uint32_t iIndex = 0;

			for (const auto &items : index)
			{
				if (nodeList[iIndex][items])
				{
					nodeList.push_back(nodeList[iIndex][items]);
					iIndex++;
				}
				else
				{
					throw std::invalid_argument("Configuration|field:" + items + "|not found");
				}
			}

			return nodeList[iIndex].as<T>();
		}
		
		template <typename T, typename S>
		T yamlAs(std::vector<std::string> index, const S& fallback)
		{
			std::lock_guard<std::mutex> guard(node_sync_);

			//YAML::Node node = YAML::Clone(node_);
			////YAML::Node node = node_;
			//for (const auto &items : index)
			//{
			//	if (node[items])
			//	{
			//		node = node[items];
			//	}
			//	else
			//	{
			//		return fallback;
			//	}
			//}

			std::vector<YAML::Node> nodeList;
			nodeList.push_back(node_);

			uint32_t iIndex = 0;

			for (const auto &items : index)
			{
				if (nodeList[iIndex][items])
				{
					nodeList.push_back(nodeList[iIndex][items]);
					iIndex++;
				}
				else
				{
					return fallback;
				}
			}

			return nodeList[iIndex].as<T>(fallback);
		}

		std::string launchTime();

		uint32_t getServerId();
		void setServerId(uint32_t id);

		static std::string logName();
		static std::string logPostfix();

    private:
		void daemonize();
		bool adjustOpenFilesLimit();
		void enableCoreFiles();
		void handleSigProcMask();

		typedef std::vector<std::shared_ptr<Event::DispatchedThreadImpl>> ThreadVec;
		std::map<Event::EThreadType, ThreadVec> thread_;

		std::shared_ptr<Event::DispatchedThreadImpl> logic_thread_;
		std::shared_ptr<Event::DispatchedThreadImpl> log_thread_;
		std::shared_ptr<Event::DispatchedThreadImpl> metrics_thread_;
		std::shared_ptr<Event::DispatchedThreadImpl> db_thread_;
		std::map<uint32_t, std::shared_ptr<Event::DispatchedThreadImpl>> thread_id_;

		std::string m_launchTime;

		std::atomic<uint32_t> tid_ = 0;
		YAML::Node node_;
		std::mutex node_sync_;

		std::shared_ptr<SelfRegistration> endpoint_ = nullptr;
		uint32_t m_server_id = 0;

        Ctx (const Ctx&) = delete;
        const Ctx &operator = (const Ctx&) = delete;

		static PlatformImpl s_platform;
		static std::string s_log_name;
		static std::string s_log_postfix;
    };
    

	//typedef APie::ThreadSafeSingleton<Ctx> CtxSingleton;

	using CtxSingleton = ThreadSafeSingleton<Ctx>;

	//usage: Envoy::CtxSingleton::get();
}

