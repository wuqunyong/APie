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

#include "nats_proxy.h"
#include "nats/adapters/libevent.h"

#include "../rpc/server/rpc_server.h"
#include "../rpc/client/rpc_client.h"

#include "../influxdb-cpp/influxdb.hpp"

namespace APie {
namespace Event {

int32_t NATSConnectorBase::ConnectBase(struct event_base* ptrBase) {
  natsOptions* nats_opts = nullptr;
  natsOptions_Create(&nats_opts);


  if (ptrBase == nullptr) 
  {
    ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "connect|event_base null");
	return 1;
  }

  natsLibevent_Init();


  if (tls_config_ != nullptr) 
  {
    natsOptions_SetSecure(nats_opts, true);
    natsOptions_LoadCATrustedCertificates(nats_opts, tls_config_->ca_cert.c_str());
    natsOptions_LoadCertificatesChain(nats_opts, tls_config_->tls_cert.c_str(),
                                      tls_config_->tls_key.c_str());
  }

  auto s = natsOptions_SetEventLoop(nats_opts, ptrBase, natsLibevent_Attach,
	  natsLibevent_Read, natsLibevent_Write, natsLibevent_Detach);

  if (s != NATS_OK) 
  {
	  std::stringstream ss;
	  ss << "Failed to set NATS event loop, nats_status=" << s;
	  ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "connect|%s", ss.str().c_str());

    return 2;
  }

  natsOptions_SetURL(nats_opts, nats_server_.c_str());


  //natsOptions_SetTimeout(nats_opts, config.connectionTimeout.count());
  //natsOptions_SetMaxReconnect(nats_opts, config.maxReconnectionAttempts);
  //natsOptions_SetReconnectWait(nats_opts, config.reconnectWait);

  natsOptions_SetClosedCB(nats_opts, ClosedCb, this);
  natsOptions_SetDisconnectedCB(nats_opts, DisconnectedCb, this);
  natsOptions_SetReconnectedCB(nats_opts, ReconnectedCb, this);
  natsOptions_SetMaxPendingMsgs(nats_opts, 65536);
  natsOptions_SetErrorHandler(nats_opts, ErrHandler, this);

  auto nats_status = natsConnection_Connect(&nats_connection_, nats_opts);
  natsOptions_Destroy(nats_opts);
  nats_opts = nullptr;

  if (nats_status != NATS_OK) 
  {
	  std::stringstream ss;
	  ss << "Failed to connect to NATS, nats_status=" << s;
	  ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "connet|%s", ss.str().c_str());
	  return 3;
  }

  ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_NOTICE, "connect success|%s", nats_server_.c_str());

  return 0;
}

void NATSConnectorBase::DisconnectedCb(natsConnection* nc, void* closure)
{
	int32_t status = -1;
	if (nc != nullptr)
	{
		status = natsConnection_Status(nc);
	}

	std::thread::id iThreadId = std::this_thread::get_id();

	std::stringstream ss;
	ss << "DisconnectedCb|status:" << status << "|threadId:" << iThreadId;
	ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_WARNING, "status|%s", ss.str().c_str());
}

void NATSConnectorBase::ReconnectedCb(natsConnection* nc, void* closure)
{
	int32_t status = -1;
	if (nc != nullptr)
	{
		status = natsConnection_Status(nc);
	}

	if (status == NATS_CONN_STATUS_CONNECTED)
	{
		auto* connector = static_cast<NATSConnectorBase*>(closure);
		connector->conn_closed = false;
	}

	std::thread::id iThreadId = std::this_thread::get_id();

	std::stringstream ss;
	ss << "ReconnectedCb|status:" << status << "|threadId:" << iThreadId;
	ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_WARNING, "status|%s", ss.str().c_str());
}

void NATSConnectorBase::ClosedCb(natsConnection* nc, void* closure)
{
	auto* connector = static_cast<NATSConnectorBase*>(closure);
	connector->conn_closed = true;

	int32_t status = -1;
	if (nc != nullptr)
	{
		status = natsConnection_Status(nc);
	}

	std::thread::id iThreadId = std::this_thread::get_id();

	std::stringstream ss;
	ss << "ClosedCb|status:" << status << "|threadId:" << iThreadId;
	ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_NOTICE, "status|%s", ss.str().c_str());
}

void NATSConnectorBase::ErrHandler(natsConnection* nc, natsSubscription* subscription, natsStatus err, void* closure)
{
	int32_t status = -1;
	if (nc != nullptr)
	{
		status = natsConnection_Status(nc);
	}

	std::stringstream ss;
	if (err == NATS_SLOW_CONSUMER) 
	{
		ss << "ErrHandler|status:" << status;
		ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "nats runtime error: slow consumer|%s", ss.str().c_str());
	}
	else 
	{
		ss << "ErrHandler|status:" << status;
		ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "nats runtime error|%s", ss.str().c_str());
	}
}

NatsManager::NatsManager() : nats_realm(nullptr)
{

}

NatsManager::~NatsManager() 
{
	if (interval_timer_)
	{
		interval_timer_->disableTimer();
		interval_timer_.reset(nullptr);
	}
}

bool NatsManager::init()
{
	auto bEnable = APie::CtxSingleton::get().yamlAs<bool>({ "nats","enable" }, false);
	if (bEnable == false)
	{
		return true;
	}

	uint32_t realm = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","realm" }, 0);
	uint32_t id = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","id" }, 0);
	uint32_t type = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","type" }, 0);


	auto nodeObj = APie::CtxSingleton::get().yamlAs<YAML::Node>({ "nats", "connections" }, YAML::Node());
	for (const auto& elems : nodeObj)
	{
		auto iType = APie::CtxSingleton::get().nodeYamlAs<uint32_t>(elems, { "subscription", "type" }, 0);
		auto sServer = APie::CtxSingleton::get().nodeYamlAs<std::string>(elems, { "subscription", "nats_server" }, "undefined");
		auto sDomains = APie::CtxSingleton::get().nodeYamlAs<std::string>(elems, { "subscription", "channel_domains" }, "undefined");

		switch (iType)
		{
		case E_NT_Realm:
		{
			nats_realm = createConnection(type, id, iType, sServer, sDomains);
			if (nats_realm == nullptr)
			{
				return false;
			}
			break;
		}
		default:
		{
			return false;
		}
		}
	}

	interval_timer_ = APie::CtxSingleton::get().getNatsThread()->dispatcherImpl()->createTimer([this]() -> void { runIntervalCallbacks(); });
	interval_timer_->enableTimer(std::chrono::milliseconds(2000));

	return true;
}

std::shared_ptr<NatsManager::PrxoyNATSConnector> NatsManager::createConnection(uint32_t serverType, uint32_t serverId, uint32_t domainsType, const std::string& urls, const std::string& domains)
{
	auto sharedPtr = std::make_shared<PrxoyNATSConnector>(urls, domains, domains);
	if (sharedPtr == nullptr)
	{
		return nullptr;
	}

	std::string channel = APie::Event::NatsManager::GetTopicChannel(serverType, serverId);
	struct event_base* ptrBase = &(APie::CtxSingleton::get().getNatsThread()->dispatcherImpl()->base());
	int32_t iRC = sharedPtr->Connect(ptrBase, channel);
	if (iRC != 0)
	{
		return nullptr;
	}


	// Attach the message handler for agent nats:
	sharedPtr->RegisterMessageHandler(std::bind(&NatsManager::NATSMessageHandler, this, domainsType, std::placeholders::_1));
	return sharedPtr;
}

void NatsManager::destroy()
{
	if (interval_timer_)
	{
		interval_timer_->disableTimer();
		interval_timer_.reset(nullptr);
	}

	if (nats_realm)
	{
		nats_realm->destroy();
		nats_realm = nullptr;
	}
}

bool NatsManager::isConnect(E_NatsType type)
{
	switch (type)
	{
	case APie::Event::NatsManager::E_NT_Realm:
	{
		if (nats_realm == nullptr)
		{
			return false;
		}

		return nats_realm->isConnect();
	}
	default:
	{
		return false;
	}
	}

	return false;
}

int32_t NatsManager::publishNatsMsg(E_NatsType type, const std::string& channel, const PrxoyNATSConnector::OriginType& msg)
{
	switch (type)
	{
	case APie::Event::NatsManager::E_NT_Realm:
	{
		if (nats_realm == nullptr)
		{
			std::stringstream ss;
			ss << "nats_realm nullptr";
			ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "publish|channel:%s|%s", channel.c_str(), ss.str().c_str());

			return 100;
		}

		return nats_realm->Publish(channel, msg);
	}
	default:
		break;
	}

	return 101;
}

void NatsManager::NATSMessageHandler(uint32_t type, PrxoyNATSConnector::MsgType msg)
{
	std::thread::id iThreadId = std::this_thread::get_id();
	ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_DEBUG, "msgHandle|threadid:%d|type:%d|%s", iThreadId, type, msg->ShortDebugString().c_str());

	if (APie::CtxSingleton::get().getLogicThread() == nullptr)
	{
		ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "msgHandle|LogicThread null|threadid:%d", iThreadId);
		return;
	}


	{
		std::lock_guard<std::mutex> guard(_sync);

		if (msg->has_rpc_request())
		{
			::rpc_msg::RPC_REQUEST request = msg->rpc_request();
			std::string sMetricsChannel = NatsManager::GetMetricsChannel(request.client().stub(), request.server().stub());
			channel_request_msgs[sMetricsChannel] = channel_request_msgs[sMetricsChannel] + 1;
		}

		if (msg->has_rpc_response())
		{
			::rpc_msg::RPC_RESPONSE response = msg->rpc_response();

			std::string sMetricsChannel = NatsManager::GetMetricsChannel(response.client().stub(), response.server().stub());
			channel_response_msgs[sMetricsChannel] = channel_response_msgs[sMetricsChannel] + 1;
		}
	}

	switch (type)
	{
	case APie::Event::NatsManager::E_NT_Realm:
	{
		::nats_msg::NATS_MSG_PRXOY* m = msg.release();
		APie::CtxSingleton::get().getLogicThread()->dispatcher().post(
			[m, this]() mutable { Handle_RealmSubscribe(std::unique_ptr<::nats_msg::NATS_MSG_PRXOY>(m)); }
		);
		break;
	}
	default:
	{
		ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "msgHandle|invalid type|threadid:%d|type:%d", iThreadId, type);
	}
	}

}

void NatsManager::runIntervalCallbacks()
{
	std::thread::id iThreadId = std::this_thread::get_id();

	bool enable = APie::CtxSingleton::get().yamlAs<bool>({ "metrics","enable" }, false);
	if (enable)
	{
		MetricData* ptrData = new MetricData;
		ptrData->sMetric = "queue";

		auto iType = APie::CtxSingleton::get().getServerType();
		auto iId = APie::CtxSingleton::get().getServerId();

		ptrData->tag["server_type"] = std::to_string(iType);
		ptrData->tag["server_id"] = std::to_string(iId);
		ptrData->tag["queue_id"] = std::to_string(iType) + "_" + std::to_string(iId) + "_nats";

		std::lock_guard<std::mutex> guard(_sync);

		uint32_t iRequestMsgs = 0;
		uint32_t iResponseMsgs = 0;
		for (auto& elems : channel_request_msgs)
		{
			iRequestMsgs += elems.second;

			std::string sChannel = "request_" + elems.first;
			ptrData->field[sChannel] = (double)elems.second;
		}

		for (auto& elems : channel_response_msgs)
		{
			iResponseMsgs += elems.second;

			std::string sChannel = "response_" + elems.first;
			ptrData->field[sChannel] = (double)elems.second;
		}
		ptrData->field["iRequestMsgs"] = (double)iRequestMsgs;
		ptrData->field["iResponseMsgs"] = (double)iResponseMsgs;

		channel_request_msgs.clear();
		channel_response_msgs.clear();

		Command command;
		command.type = Command::metric_data;
		command.args.metric_data.ptrData = ptrData;

		auto ptrMetric = APie::CtxSingleton::get().getMetricsThread();
		if (ptrMetric != nullptr)
		{
			ptrMetric->push(command);
		}
	}

	interval_timer_->enableTimer(std::chrono::milliseconds(1000));
}

void NatsManager::Handle_RealmSubscribe(std::unique_ptr<::nats_msg::NATS_MSG_PRXOY> msg)
{
	std::thread::id iThreadId = std::this_thread::get_id();
	ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_DEBUG, "Handle_Subscribe|threadid:%d|%s", iThreadId, msg->ShortDebugString().c_str());

	if (msg->has_rpc_request())
	{
		::rpc_msg::RPC_REQUEST request = msg->rpc_request();

		std::string channel = NatsManager::GetTopicChannel(request.client().stub().type(), request.client().stub().id());

		::rpc_msg::CHANNEL server;
		server.set_type(APie::CtxSingleton::get().identify().type);
		server.set_id(APie::CtxSingleton::get().identify().id);

		if (request.server().stub().type() != server.type() || request.server().stub().id() != server.id())
		{
			ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "msgHandle|has_rpc_request||cur server:%s|%s", server.ShortDebugString().c_str(), msg->ShortDebugString().c_str());
			return;
		}

		request.mutable_client()->set_channel_serial_num(0);

		::rpc_msg::RPC_RESPONSE response;
		*response.mutable_client() = request.client();
		*response.mutable_server()->mutable_stub() = server;


		auto functionOpt = APie::RPC::RpcServerSingleton::get().findFunction(request.opcodes());
		if (!functionOpt.has_value())
		{
			response.mutable_status()->set_code(::rpc_msg::RPC_CODE::CODE_Unregister);

			::nats_msg::NATS_MSG_PRXOY nats_msg;
			(*nats_msg.mutable_rpc_response()) = response;
			APie::Event::NatsSingleton::get().publishNatsMsg(APie::Event::NatsManager::E_NT_Realm, channel, nats_msg);

			return;
		}

		auto typeOpt = APie::RPC::RpcServerSingleton::get().getType(request.opcodes());
		if (!typeOpt.has_value())
		{
			response.mutable_status()->set_code(::rpc_msg::RPC_CODE::CODE_Unregister);

			::nats_msg::NATS_MSG_PRXOY nats_msg;
			(*nats_msg.mutable_rpc_response()) = response;
			APie::Event::NatsSingleton::get().publishNatsMsg(APie::Event::NatsManager::E_NT_Realm, channel, nats_msg);
			return;
		}

		std::string sType = typeOpt.value();
		auto ptrMsg = Api::PBHandler::createMessage(sType);
		if (ptrMsg == nullptr)
		{
			return;
		}

		std::shared_ptr<::google::protobuf::Message> newMsg(ptrMsg);
		bool bResult = newMsg->ParseFromString(request.args_data());
		if (!bResult)
		{
			return;
		}

		auto tupleResult = functionOpt.value()(request.client(), newMsg.get());

		uint32_t iCode = std::get<0>(tupleResult);
		if (iCode == ::rpc_msg::RPC_CODE::CODE_Ok_Async)
		{
			return;
		}

		if (!request.client().required_reply())
		{
			return;
		}

		response.mutable_status()->set_code(iCode);
		response.set_result_data(std::get<1>(tupleResult));

		::nats_msg::NATS_MSG_PRXOY nats_msg;
		(*nats_msg.mutable_rpc_response()) = response;
		APie::Event::NatsSingleton::get().publishNatsMsg(APie::Event::NatsManager::E_NT_Realm, channel, nats_msg);

		return;
	}

	if (msg->has_rpc_response())
	{
		::rpc_msg::RPC_RESPONSE response = msg->rpc_response();

		::rpc_msg::CHANNEL server;
		server.set_type(APie::CtxSingleton::get().identify().type);
		server.set_id(APie::CtxSingleton::get().identify().id);

		if (response.client().stub().type() != server.type() || response.client().stub().id() != server.id())
		{
			ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "msgHandle|has_rpc_response|invalid target|cur server:%s|%s", server.DebugString().c_str(), msg->DebugString().c_str());
			return;
		}

		uint64_t seqId = response.client().seq_id();
		auto replyCb = APie::RPC::RpcClientSingleton::get().find(seqId);
		if (replyCb == nullptr)
		{
			//TODO
			return;
		}

		bool hasMore = false;
		auto status = response.status();

		bool isStream = APie::RPC::RpcClientSingleton::get().isServerStream(seqId);
		if (isStream)
		{
			hasMore = response.has_more();

			status.set_has_more(hasMore);
			status.set_offset(response.offset());
		}

		replyCb(status, response.result_data());

		if (hasMore)
		{
			//Nothing
		}
		else
		{
			APie::RPC::RpcClientSingleton::get().del(seqId);
		}
		APie::RPC::RpcClientSingleton::get().handleTimeout();
	}
}

std::string NatsManager::GetTopicChannel(uint32_t type, uint32_t id)
{
	std::string channel = std::to_string(type) + "/" + std::to_string(id);
	return channel;
}

std::string NatsManager::GetMetricsChannel(const ::rpc_msg::CHANNEL& src, const ::rpc_msg::CHANNEL& dest)
{
	std::string channel = std::to_string(src.type()) + "/" + std::to_string(src.id()) + "->" + std::to_string(dest.type()) + "/" + std::to_string(dest.id());
	return channel;
}

}  // namespace APie
}  // namespace Event
