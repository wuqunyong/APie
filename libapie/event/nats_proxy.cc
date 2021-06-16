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
#include "adapters/libevent.h"

#include "../rpc/server/rpc_server.h"
#include "../rpc/client/rpc_client.h"

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

  natsOptions_SetClosedCB(nats_opts, NATSConnectorBase::ClosedCb, this);
  natsOptions_SetDisconnectedCB(nats_opts, NATSConnectorBase::DisconnectedCb, this);
  natsOptions_SetReconnectedCB(nats_opts, NATSConnectorBase::ReconnectedCb, this);


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

  return 0;
}

void NATSConnectorBase::DisconnectedCb(natsConnection* nc, void* closure)
{
	int32_t status = -1;
	if (nc != nullptr)
	{
		status = natsConnection_Status(nc);
	}

	std::stringstream ss;
	ss << "DisconnectedCb|status:" << status;
	ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "status|%s", ss.str().c_str());
}

void NATSConnectorBase::ReconnectedCb(natsConnection* nc, void* closure)
{
	int32_t status = -1;
	if (nc != nullptr)
	{
		status = natsConnection_Status(nc);
	}

	std::stringstream ss;
	ss << "ReconnectedCb|status:" << status;
	ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "status|%s", ss.str().c_str());
}

void NATSConnectorBase::ClosedCb(natsConnection* nc, void* closure)
{
	int32_t status = -1;
	if (nc != nullptr)
	{
		status = natsConnection_Status(nc);
	}

	std::stringstream ss;
	ss << "ClosedCb|status:" << status;
	ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_ERROR, "status|%s", ss.str().c_str());
}


NatsManager::NatsManager() : nats_proxy(nullptr)
{

}

NatsManager::~NatsManager() 
{
}

bool NatsManager::init()
{
	auto bEnable = APie::CtxSingleton::get().yamlAs<bool>({ "nats","enable" }, false);
	if (bEnable == false)
	{
		return true;
	}

	uint32_t id = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","id" }, 0);
	uint32_t type = APie::CtxSingleton::get().yamlAs<uint32_t>({ "identify","type" }, 0);

	auto sServer = APie::CtxSingleton::get().yamlAs<std::string>({ "nats", "nats_server" }, "");
	auto sSub = APie::CtxSingleton::get().yamlAs<std::string>({ "nats", "sub_topic" }, "");
	auto sPub = APie::CtxSingleton::get().yamlAs<std::string>({ "nats", "pub_topic" }, "");

	nats_proxy = std::make_shared<PrxoyNATSConnector>(sServer, sPub, sSub);
	if (nats_proxy == nullptr)
	{
		return false;
	}

	std::string sPostfix = std::to_string(type) + ":" + std::to_string(id);
	struct event_base* ptrBase = &(APie::CtxSingleton::get().getLogicThread()->dispatcherImpl()->base());
	int32_t iRC = nats_proxy->Connect(ptrBase, sPostfix);
	if (iRC != 0)
	{
		return false;
	}


	// Attach the message handler for agent nats:
	nats_proxy->RegisterMessageHandler(std::bind(&NatsManager::NATSMessageHandler, this, std::placeholders::_1));

	return true;
}

bool NatsManager::inConnect()
{
	if (nats_proxy == nullptr)
	{
		return false;
	}

	return nats_proxy->isConnect();
}

void NatsManager::NATSMessageHandler(PrxoyNATSConnector::MsgType msg)
{
	//::nats_msg::NATS_MSG_PRXOY* m = msg.release();

	ASYNC_PIE_LOG("nats/proxy", PIE_CYCLE_HOUR, PIE_DEBUG, "msgHandle|%s", msg->DebugString().c_str());

	if (msg->has_rpc_request())
	{
		::rpc_msg::RPC_REQUEST request = msg->rpc_request();

		std::string channel = std::to_string(request.client().stub().type()) + ":" + std::to_string(request.client().stub().id());

		::rpc_msg::CHANNEL server;
		server.set_type(APie::CtxSingleton::get().identify().type);
		server.set_id(APie::CtxSingleton::get().identify().id);

		if (request.server().stub().type() != server.type() || request.server().stub().id() != server.id())
		{
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
			APie::Event::NatsSingleton::get().publish(channel, nats_msg);

			return;
		}

		auto typeOpt = APie::RPC::RpcServerSingleton::get().getType(request.opcodes());
		if (!typeOpt.has_value())
		{
			response.mutable_status()->set_code(::rpc_msg::RPC_CODE::CODE_Unregister);

			::nats_msg::NATS_MSG_PRXOY nats_msg;
			(*nats_msg.mutable_rpc_response()) = response;
			APie::Event::NatsSingleton::get().publish(channel, nats_msg);
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
		APie::Event::NatsSingleton::get().publish(channel, nats_msg);

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

}  // namespace APie
}  // namespace Event
