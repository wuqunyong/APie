/*
 * Copyright 2018- The Pixie Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "nats/nats.h"

#include <memory>
#include <string>
#include <thread>

#include "../network/logger.h"
#include "../pb_msg.h"

namespace APie {
namespace Event {

		class NatsManager;
		
		struct NATSTLSConfig {
			std::string ca_cert;
			std::string tls_key;
			std::string tls_cert;
		};

		class NATSConnectorBase {
		public:
			static void DisconnectedCb(natsConnection* nc, void* closure);
			static void ReconnectedCb(natsConnection* nc, void* closure);
			static void ClosedCb(natsConnection* nc, void* closure);
			static void ErrHandler(natsConnection* nc, natsSubscription* subscription, natsStatus err, void* closure);

		protected:
			NATSConnectorBase(std::string nats_server, std::unique_ptr<NATSTLSConfig> tls_config)
				: nats_server_(nats_server), tls_config_(std::move(tls_config)) {}

			int32_t ConnectBase(struct event_base* ptrBase);

			natsConnection* nats_connection_ = nullptr;

			std::string nats_server_;
			std::unique_ptr<NATSTLSConfig> tls_config_;

			bool conn_closed = false;
		};


		/**
		 * NATS connector for a single topic.
		 * @tparam TMsg message type. Must be a protobuf.
		 */
		template <typename TMsg>
		class NATSConnector : public NATSConnectorBase {
		public:
			using OriginType = TMsg;
			using MsgType = std::unique_ptr<TMsg>;
			using MessageHandlerCB = std::function<void(MsgType)>;

			NATSConnector(std::string nats_server, std::string pub_topic, std::string sub_topic)
				: NATSConnectorBase(nats_server, nullptr),
				pub_topic_(pub_topic),
				sub_topic_(sub_topic) {}

			virtual ~NATSConnector()
			{
				if (nats_subscription_ != nullptr)
				{
					natsSubscription_Destroy(nats_subscription_);
				}

				if (nats_connection_ != nullptr)
				{
					natsConnection_Destroy(nats_connection_);
				}

				nats_Close();
			}

			void destroy()
			{
				if (nats_subscription_)
				{
					// Remove interest from the subscription. Note that pending message may still
					// be received by the client.
					natsStatus status;
					status = natsSubscription_Unsubscribe(nats_subscription_);
					if (status != NATS_OK) 
					{
						ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_NOTICE, "Failed to unsubscribe");
					}

					status = natsSubscription_Drain(nats_subscription_);
					if (status != NATS_OK) 
					{
						ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_NOTICE, "Failed to drain subscription");
					}

					status = natsSubscription_WaitForDrainCompletion(nats_subscription_, 1000);
					if (status != NATS_OK) 
					{
						ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_NOTICE, "Failed to wait for subscription drain");
					}
					// Called only here, because it needs to wait for the natsSubscription_WaitForDrainCompletion
					natsSubscription_Destroy(nats_subscription_);
					nats_subscription_ = nullptr;
				}

				natsConnection_Close(nats_connection_);
				while (!conn_closed) 
				{
					// Wait until the connection is actually closed. This will be reported on a different
					// thread.
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
				natsConnection_Destroy(nats_connection_);
				nats_connection_ = nullptr;
			}

			/**
			 * Connect to the nats server.
			 * @return Status of the connection.
			 */
			virtual int32_t Connect(struct event_base* ptrBase, const std::string& channel) 
			{
				int32_t iRC = ConnectBase(ptrBase);
				if (iRC != 0)
				{
					return iRC;
				}

				std::string sSub = APie::Event::NatsManager::GetCombineTopicChannel(sub_topic_, channel);

				// Attach the message reader.
				natsStatus status = natsConnection_Subscribe(&nats_subscription_, nats_connection_, sSub.c_str(), NATSMessageCallbackHandler, this);
				PIE_LOG("startup/startup", PIE_CYCLE_HOUR, PIE_NOTICE, "nats|subscribe|%s|%s|%d", nats_server_.c_str(), sSub.c_str(), status);
				
				return status;
			}

			/**
			 * Handle incoming message from NATS. This function is used by the C callback function. It can
			 * also be used by Fakes/tests to inject new messages.
			 * @param msg The natsMessage.
			 */
			void NATSMessageHandler(natsConnection* /*nc*/, natsSubscription* /*sub*/, natsMsg* msg)
			{
				if (msg == nullptr)
				{
					std::stringstream ss;
					ss << "topic:" << sub_topic_ << "timeout subscription";
					ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "handleMSG|%s", ss.str().c_str());
					return;
				}

				int len = natsMsg_GetDataLength(msg);
				const char* data = natsMsg_GetData(msg);
				auto parsed_msg = std::make_unique<TMsg>();

				bool ok = parsed_msg->ParseFromArray(data, len);
				if (!ok)
				{
					std::stringstream ss;
					ss << "topic:" << sub_topic_ << "Failed to parse message";
					ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "handleMSG|%s", ss.str().c_str());

					natsMsg_Destroy(msg);
					return;
				}

				if (msg_handler_)
				{
					msg_handler_(std::move(parsed_msg));
				}
				else
				{
					std::stringstream ss;
					ss << "Dropping message (no handler registered): " << parsed_msg->DebugString();
					ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_WARNING, "handleMSG|%s", ss.str().c_str());
				}

				natsMsg_Destroy(msg);
			}

			/**
			 * Publish a message to the NATS topic.
			 * @param msg The protobuf message.
			 * @return Status of publication.
			 */
			virtual int32_t Publish(const std::string& channel, const TMsg& msg)
			{
				if (!nats_connection_)
				{
					std::stringstream ss;
					ss << "Not connected to NATS";

					ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "handlePublish|%s", ss.str().c_str());
					return 1;
				}

				std::string sPub = APie::Event::NatsManager::GetCombineTopicChannel(pub_topic_, channel);

				auto serialized_msg = msg.SerializeAsString();
				auto nats_status = natsConnection_Publish(nats_connection_, sPub.c_str(), serialized_msg.c_str(), serialized_msg.size());
				if (nats_status != NATS_OK)
				{
					std::stringstream ss;
					ss << "Failed to publish to NATS, nats_status=" << nats_status;
					ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "handlePublish|%s", ss.str().c_str());

					return 2;
				}

				return 0;
			}

			/**
			 * Register the message handler.
			 * The lifetime of the handler (and bound variables), must exceed the lifetime of this class,
			 * or RemoveMessageHandler must be called.
			 */
			void RegisterMessageHandler(MessageHandlerCB handler) { msg_handler_ = std::move(handler); }

			/**
			 * Remove the message handler if attached.
			 */
			void RemoveMessageHandler() { msg_handler_ = nullptr; }

			bool isConnect()
			{
				if (nats_connection_ == nullptr)
				{
					std::stringstream ss;
					ss << "nats_connection_ nullptr";
					ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "isConnect|%s", ss.str().c_str());

					return false;
				}

				auto connStatus = natsConnection_Status(nats_connection_);
				if (connStatus != NATS_CONN_STATUS_CONNECTED)
				{
					std::stringstream ss;
					ss << "status:" << connStatus;
					ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "isConnect|%s", ss.str().c_str());

					return false;
				}

				return true;
			}

		protected:
			static void NATSMessageCallbackHandler(natsConnection* nc, natsSubscription* sub, natsMsg* msg,
				void* closure)
			{
				// We know that closure is of type NATSConnector.
				auto* connector = static_cast<NATSConnector<TMsg>*>(closure);
				connector->NATSMessageHandler(nc, sub, msg);
			}

			natsSubscription* nats_subscription_ = nullptr;

			std::string pub_topic_;
			std::string sub_topic_;

			// The registered message handler.
			MessageHandlerCB msg_handler_;
		};


		class NatsManager {
		public:
			using PrxoyNATSConnector = APie::Event::NATSConnector<::nats_msg::NATS_MSG_PRXOY>;

			NatsManager();
			~NatsManager();

			enum E_NatsType
			{
				E_NT_None = 0,
				E_NT_Realm = 1,
			};

			bool init();
			void destroy();

		public:
			bool isConnect(E_NatsType type);
			int32_t publishNatsMsg(E_NatsType type, const std::string& channel, const PrxoyNATSConnector::OriginType& msg);

		public:
			static std::string GetTopicChannel(uint32_t realm, uint32_t type, uint32_t id);
			static std::string GetCombineTopicChannel(const std::string& domains, const std::string& channel);

			static std::string GetMetricsChannel(const ::rpc_msg::CHANNEL& src, const ::rpc_msg::CHANNEL& dest);

			void Handle_RealmSubscribe(std::unique_ptr<::nats_msg::NATS_MSG_PRXOY> msg);

		private:
			void NATSMessageHandler(uint32_t type, PrxoyNATSConnector::MsgType msg);
			void runIntervalCallbacks();

			std::shared_ptr<PrxoyNATSConnector> createConnection(uint32_t realm, uint32_t serverType, uint32_t serverId, uint32_t domainsType, const std::string& urls, const std::string& domains);

		private:
			NatsManager(const NatsManager&) = delete;
			NatsManager& operator=(const NatsManager&) = delete;

			std::shared_ptr<PrxoyNATSConnector> nats_realm;

			std::mutex _sync;
			std::map<std::string, uint32_t> channel_request_msgs;
			std::map<std::string, uint32_t> channel_response_msgs;

			TimerPtr interval_timer_ = nullptr;

		};

		using NatsSingleton = ThreadSafeSingleton<NatsManager>;
}  // namespace Event
}  // namespace APie
