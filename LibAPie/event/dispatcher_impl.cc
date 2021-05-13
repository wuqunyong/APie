#include "../event/dispatcher_impl.h"

#include <ctime>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <cassert>
#include <assert.h>

#include "../event/libevent_scheduler.h"
#include "../event/signal_impl.h"
#include "../event/timer_impl.h"

#include "../network/listener.h"
#include "../network/listener_impl.h"
#include "../network/server_connection.h"
#include "../network/ctx.h"
#include "../network/i_poll_events.hpp"
#include "../network/client_proxy.h"
#include "../network/logger.h"
#include "../network/end_point.h"

#include "../api/api.h"
#include "../api/pb_handler.h"
#include "../api/hook.h"
#include "../api/pubsub.h"

#include "../common/string_utils.h"

#include "../rpc/client/rpc_client.h"
#include "../rpc/server/rpc_server.h"

#include "event2/event.h"
#include "influxdb.hpp"
#include "../compressor/lz4_compressor_impl.h"
#include "../crypto/crypto_utility.h"
#include "../redis_driver/redis_client.h"


namespace APie {
namespace Event {

std::atomic<uint64_t> DispatcherImpl::serial_num_(0);
std::mutex DispatcherImpl::connecton_sync_;
std::map<uint64_t, std::shared_ptr<ServerConnection>> DispatcherImpl::connection_map_;
std::map<uint64_t, std::shared_ptr<ClientConnection>> DispatcherImpl::client_connection_map_;

DispatcherImpl::DispatcherImpl(EThreadType type, uint32_t tid)
	: type_(type),
	tid_(tid),
	deferred_delete_timer_(createTimer([this]() -> void { clearDeferredDeleteList(); })),
	post_timer_(createTimer([this]() -> void { runPostCallbacks(); })),
	interval_timer_(createTimer([this]() -> void { runIntervalCallbacks(); })),
	current_to_delete_(&to_delete_1_),
	i_next_check_rotate(0),
	i_next_metric_time(0),
	terminating_(false)
{

}

DispatcherImpl::~DispatcherImpl() {}

void DispatcherImpl::clearDeferredDeleteList() {
  std::vector<DeferredDeletablePtr>* to_delete = current_to_delete_;

  size_t num_to_delete = to_delete->size();
  if (deferred_deleting_ || !num_to_delete) {
    return;
  }


  // Swap the current deletion vector so that if we do deferred delete while we are deleting, we
  // use the other vector. We will get another callback to delete that vector.
  if (current_to_delete_ == &to_delete_1_) {
    current_to_delete_ = &to_delete_2_;
  } else {
    current_to_delete_ = &to_delete_1_;
  }

  deferred_deleting_ = true;

  // Calling clear() on the vector does not specify which order destructors run in. We want to
  // destroy in FIFO order so just do it manually. This required 2 passes over the vector which is
  // not optimal but can be cleaned up later if needed.
  for (size_t i = 0; i < num_to_delete; i++) {
    (*to_delete)[i].reset();
  }

  to_delete->clear();
  deferred_deleting_ = false;
}

Network::ListenerPtr DispatcherImpl::createListener(Network::ListenerCbPtr cb, Network::ListenerConfig config) {
  return Network::ListenerPtr{new Network::ListenerImpl(*this, cb, config)};
}

TimerPtr DispatcherImpl::createTimer(TimerCb cb) {
  return base_scheduler_.createTimer(cb);
}

void DispatcherImpl::deferredDelete(DeferredDeletablePtr&& to_delete) {
  current_to_delete_->emplace_back(std::move(to_delete));
  if (1 == current_to_delete_->size()) {
    deferred_delete_timer_->enableTimer(std::chrono::milliseconds(0));
  }
}

void DispatcherImpl::start()
{
	mailbox_.registerFd(&base_scheduler_.base(), processCommand, this);
}

void DispatcherImpl::exit() 
{ 
	base_scheduler_.loopExit();

	switch (type_)
	{
	case APie::Event::EThreadType::TT_Logic:
	{
		RedisClientFactorySingleton::get().destroy();
		break;
	}
	default:
		break;
	}
}

SignalEventPtr DispatcherImpl::listenForSignal(int signal_num, SignalCb cb) {
  return SignalEventPtr{new SignalEventImpl(*this, signal_num, cb)};
}

void DispatcherImpl::post(std::function<void()> callback) {
  bool do_post;
  {
	std::lock_guard<std::mutex> lock(post_lock_);
    do_post = post_callbacks_.empty();
    post_callbacks_.push_back(callback);
  }

  if (do_post) {
    post_timer_->enableTimer(std::chrono::milliseconds(0));
  }
}

void DispatcherImpl::run(void) {
  // Flush all post callbacks before we run the event loop. We do this because there are post
  // callbacks that have to get run before the initial event loop starts running. libevent does
  // not guarantee that events are run in any particular order. So even if we post() and call
  // event_base_once() before some other event, the other event might get called first.
  runPostCallbacks();
  interval_timer_->enableTimer(std::chrono::milliseconds(200));
  base_scheduler_.run();
}

void DispatcherImpl::push(Command& cmd)
{
	mailbox_.send(cmd);
}

std::atomic<bool>& DispatcherImpl::terminating()
{
	return this->terminating_;
}

void DispatcherImpl::runIntervalCallbacks()
{
	bool enable = APie::CtxSingleton::get().yamlAs<bool>({"metrics","enable"}, false);
	if (enable)
	{
		MetricData *ptrData = new MetricData;
		ptrData->sMetric = "queue";

		auto iType = APie::CtxSingleton::get().getServerType();
		auto iId = APie::CtxSingleton::get().getServerId();

		ptrData->tag["server_type"] = std::to_string(iType);
		ptrData->tag["server_id"] = std::to_string(iId);
		ptrData->tag["thread_type"] = toStirng(type_);
		ptrData->tag["thread_id"] = std::to_string(tid_);
		ptrData->tag["queue_id"] = std::to_string(iType) + "_" + std::to_string(iId) + "_" + toStirng(type_) + "_" + std::to_string(tid_);

		uint32_t iCmdType = m_cmdStats.size();
		uint32_t iCmdCount = 0;
		for (auto& elems : m_cmdStats)
		{
			iCmdCount += elems.second;

			std::string sCmd = "cmd_type_" + std::to_string(elems.first);
			ptrData->field[sCmd] = (double)elems.second;
		}
		ptrData->field["cmd_type"] = (double)iCmdType;
		ptrData->field["cmd_count"] = (double)iCmdCount;
		m_cmdStats.clear();

		uint32_t iPbType = m_pbStats.size();
		uint32_t iPbCount = 0;
		for (auto& elems : m_pbStats)
		{
			iCmdCount += elems.second;

			std::string sPb = "pb_type_" + std::to_string(elems.first);
			ptrData->field[sPb] = (double)elems.second;
		}
		ptrData->field["pb_type"] = (double)iPbType;
		ptrData->field["pb_count"] = (double)iPbCount;
		m_pbStats.clear();

		ptrData->field["mailbox"] = (double)mailbox_.size();
		ptrData->field["timer_call"] = (double)TimerImpl::s_callCount;

		TimerImpl::s_callCount = 0;

		Command command;
		command.type = Command::metric_data;
		command.args.metric_data.ptrData = ptrData;

		auto ptrMetric = APie::CtxSingleton::get().getMetricsThread();
		if (ptrMetric != nullptr)
		{
			ptrMetric->push(command);
		}
	}

	switch (type_)
	{
	case APie::Event::EThreadType::TT_Logic:
	{
		APie::RPC::RpcClientSingleton::get().handleTimeout();
		break;
	}
	default:
		break;
	}

	interval_timer_->enableTimer(std::chrono::milliseconds(1000));
}

void DispatcherImpl::runPostCallbacks() {
  while (true) {
    // It is important that this declaration is inside the body of the loop so that the callback is
    // destructed while post_lock_ is not held. If callback is declared outside the loop and reused
    // for each iteration, the previous iteration's callback is destructed when callback is
    // re-assigned, which happens while holding the lock. This can lead to a deadlock (via
    // recursive mutex acquisition) if destroying the callback runs a destructor, which through some
    // callstack calls post() on this dispatcher.
    std::function<void()> callback;
    {
	  std::lock_guard<std::mutex> lock(post_lock_);
      if (post_callbacks_.empty()) {
        return;
      }
      callback = post_callbacks_.front();
      post_callbacks_.pop_front();
    }
    callback();
  }
}

void DispatcherImpl::handleCommand()
{
	time_t iCurTime = time(NULL);
	size_t iLoopCount = mailbox_.size();

	while (iLoopCount > 0)
	{
		iLoopCount--;

		Command cmd;
		int iResult = mailbox_.recv(cmd);
		if (iResult != 0)
		{
			break;
		}

		m_cmdStats[cmd.type] = m_cmdStats[cmd.type] + 1;

		switch (cmd.type)
		{
		case Command::passive_connect:
		{
			this->handleNewConnect(cmd.args.passive_connect.ptrData);
			break;
		}
		case Command::pb_reqeust:
		{
			this->handlePBRequest(cmd.args.pb_reqeust.ptrData);
			break;
		}
		case Command::pb_forward:
		{
			this->handlePBForward(cmd.args.pb_forward.ptrData);
			break;
		}
		case Command::send_data:
		{
			this->handleSendData(cmd.args.send_data.ptrData);
			break;
		}
		case Command::send_data_by_flag:
		{
			this->handleSendDataByFlag(cmd.args.send_data_by_flag.ptrData);
			break;
		}
		case Command::async_log:
		{
			this->handleRotate(iCurTime);
			this->handleAsyncLog(cmd.args.async_log.ptrData);
			break;
		}
		case Command::metric_data:
		{
			this->handleMetric(cmd.args.metric_data.ptrData);
			break;
		}
		case Command::dial:
		{
			this->handleDial(cmd.args.dial.ptrData);
			break;
		}
		case Command::dial_result:
		{
			this->handleDialResult(cmd.args.dial_result.ptrData);
			break;
		}
		case Command::set_server_session_attr:
		{
			this->handleSetServerSessionAttr(cmd.args.set_server_session_attr.ptrData);
			break;
		}
		case Command::set_client_session_attr:
		{
			this->handleSetClientSessionAttr(cmd.args.set_client_session_attr.ptrData);
			break;
		}
		case Command::logic_cmd:
		{
			this->handleLogicCmd(cmd.args.logic_cmd.ptrData);
			break;
		}
		case Command::logic_async_call_functor:
		{
			this->handleAsyncCallFunctor(cmd.args.logic_async_call_functor.ptrData);
			break;
		}
		case Command::close_local_client:
		{
			this->handleCloseLocalClient(cmd.args.close_local_client.ptrData);
			break;
		}
		case Command::close_local_server:
		{
			this->handleCloseLocalServer(cmd.args.close_local_server.ptrData);
			break;
		}
		case Command::client_peer_close:
		{
			this->handleClientPeerClose(cmd.args.client_peer_close.ptrData);
			break;
		}
		case Command::server_peer_close:
		{
			this->handleServerPeerClose(cmd.args.server_peer_close.ptrData);
			break;
		}
		case Command::logic_start:
		{
			this->handleLogicStart(cmd.args.logic_exit.iThreadId);
			break;
		}
		case Command::logic_exit:
		{
			this->handleLogicExit(cmd.args.logic_exit.iThreadId);
			break;
		}
		case Command::stop_thread:
		{
			this->handleStopThread(cmd.args.stop_thread.iThreadId);
			break;
		}
		case Command::recv_http_request:
		{
			break;
		}
		case Command::send_http_response:
		{
			break;
		}
		case Command::recv_http_response:
		{
			break;
		}
		default:
		{
			assert(false);
		}
		}

		//  The assumption here is that each command is processed once only,
		//  so deallocating it after processing is all right.
		deallocateCommand(&cmd);
	}
}

void DispatcherImpl::processCommand(evutil_socket_t fd, short event, void *arg)
{
	((DispatcherImpl*)arg)->handleCommand();
}

uint64_t DispatcherImpl::generatorSerialNum()
{
	++serial_num_;
	return serial_num_;
}


void DispatcherImpl::addConnection(std::shared_ptr<ServerConnection> ptrConnection)
{
	std::lock_guard<std::mutex> guard(connecton_sync_);
	connection_map_[ptrConnection->getSerialNum()] = ptrConnection;
}

std::shared_ptr<ServerConnection> DispatcherImpl::getConnection(uint64_t iSerialNum)
{
	std::lock_guard<std::mutex> guard(connecton_sync_);
	auto findIte = connection_map_.find(iSerialNum);
	if (findIte == connection_map_.end())
	{
		return nullptr;
	}

	return findIte->second;
}

void DispatcherImpl::delConnection(uint64_t iSerialNum)
{
	std::lock_guard<std::mutex> guard(connecton_sync_);
	connection_map_.erase(iSerialNum);
}


void DispatcherImpl::addClientConnection(std::shared_ptr<ClientConnection> ptrConnection)
{
	std::shared_ptr<ClientConnection> delClient = nullptr;
	
	{
		std::lock_guard<std::mutex> guard(connecton_sync_);
		auto iSerialNum = ptrConnection->getSerialNum();
		auto findIte = client_connection_map_.find(iSerialNum);
		if (findIte != client_connection_map_.end())
		{
			delClient = findIte->second;
		}
	}

	if (delClient)
	{
		delClient->close("duplicate add", 0, 0);
	}

	std::lock_guard<std::mutex> guard(connecton_sync_);
	client_connection_map_[ptrConnection->getSerialNum()] = ptrConnection;
}

std::shared_ptr<ClientConnection> DispatcherImpl::getClientConnection(uint64_t iSerialNum)
{
	std::lock_guard<std::mutex> guard(connecton_sync_);
	auto findIte = client_connection_map_.find(iSerialNum);
	if (findIte == client_connection_map_.end())
	{
		return nullptr;
	}

	return findIte->second;
}

void DispatcherImpl::delClientConnection(uint64_t iSerialNum)
{
	std::lock_guard<std::mutex> guard(connecton_sync_);
	client_connection_map_.erase(iSerialNum);
}

void DispatcherImpl::clearAllConnection()
{
	std::lock_guard<std::mutex> guard(connecton_sync_);
	client_connection_map_.clear();
	connection_map_.clear();
}

static void readcb(struct bufferevent *bev, void *arg)
{
	ServerConnection* ptrConnection = (ServerConnection *)arg;
	ptrConnection->readcb();
}

static void writecb(struct bufferevent *bev, void *arg)
{
	ServerConnection *ptrConnection = (ServerConnection *)arg;
	ptrConnection->writecb();
}

static void eventcb(struct bufferevent *bev, short what, void *arg)
{
	ServerConnection *ptrConnection = (ServerConnection *)arg;
	ptrConnection->eventcb(what);
}

void DispatcherImpl::handleNewConnect(PassiveConnect *itemPtr)
{
	struct bufferevent *bev;
	bev = bufferevent_socket_new(&base_scheduler_.base(), itemPtr->iFd, BEV_OPT_CLOSE_ON_FREE);
	if (!bev)
	{
		evutil_closesocket(itemPtr->iFd);
		return;
	}

	uint64_t iSerialNum = generatorSerialNum();
	auto ptrConnection = std::make_shared<ServerConnection>(tid_, iSerialNum, bev, itemPtr->iType);
	if (nullptr == ptrConnection)
	{
		bufferevent_free(bev);
		return;
	}
	ptrConnection->setIp(itemPtr->sIp, itemPtr->sPeerIp);
	ptrConnection->setMaskFlag(itemPtr->iMaskFlag);

	bufferevent_setcb(bev, readcb, writecb, eventcb, ptrConnection.get());
	bufferevent_enable(bev, EV_READ | EV_WRITE);

	struct timeval tv_read;
	tv_read.tv_sec = 600;
	tv_read.tv_usec = 0;
	struct timeval tv_write;
	tv_write.tv_sec = 600;
	tv_write.tv_usec = 0;
	bufferevent_set_timeouts(bev, &tv_read, &tv_write);

	DispatcherImpl::addConnection(ptrConnection);

	std::stringstream ss;
	ss << "iSerialNum:" << iSerialNum << "|fd:" << itemPtr->iFd << "|iType:" << static_cast<uint32_t>(itemPtr->iType) << "|peerIp:" << itemPtr->sPeerIp << " -> " << "ip:" << itemPtr->sIp;
	ASYNC_PIE_LOG("DispatcherImpl/handleNewConnect", PIE_CYCLE_HOUR, PIE_NOTICE, "%s", ss.str().c_str());
}

void DispatcherImpl::handlePBRequest(PBRequest *itemPtr)
{
	switch (itemPtr->type)
	{
	case APie::ConnetionType::CT_SERVER:
	{
		m_pbStats[itemPtr->iOpcode] = m_pbStats[itemPtr->iOpcode] + 1;

		auto optionalData = Api::OpcodeHandlerSingleton::get().server.getFunction(itemPtr->iOpcode);
		if (!optionalData)
		{
			return;
		}

		optionalData.value()(itemPtr->iSerialNum, itemPtr->ptrMsg.get());
		break;
	}
	case APie::ConnetionType::CT_CLIENT:
	{
		m_pbStats[itemPtr->iOpcode] = m_pbStats[itemPtr->iOpcode] + 1;

		auto optionalData = Api::OpcodeHandlerSingleton::get().client.getFunction(itemPtr->iOpcode);
		if (!optionalData)
		{
			return;
		}

		optionalData.value()(itemPtr->iSerialNum, itemPtr->ptrMsg.get());
		break;
	}
	default:
		break;
	}
}

void DispatcherImpl::handlePBForward(PBForward *itemPtr)
{
	switch (itemPtr->type)
	{
	case APie::ConnetionType::CT_SERVER:
	{
		m_pbStats[itemPtr->iOpcode] = m_pbStats[itemPtr->iOpcode] + 1;

		auto defaultHandler = Api::OpcodeHandlerSingleton::get().server.getDefaultFunc();
		if (!defaultHandler)
		{
			std::stringstream ss;
			ss << "iSerialNum:" << itemPtr->iSerialNum << "|type:" << (uint32_t)itemPtr->type << "|iOpcode:" << itemPtr->iOpcode << "|unregister";
			ASYNC_PIE_LOG("handlePBForward/handlePBForward", PIE_CYCLE_HOUR, PIE_ERROR, "%s", ss.str().c_str());
			return;
		}

		defaultHandler(itemPtr->iSerialNum, itemPtr->iOpcode, itemPtr->sMsg);
		break;
	}
	case APie::ConnetionType::CT_CLIENT:
	{
		m_pbStats[itemPtr->iOpcode] = m_pbStats[itemPtr->iOpcode] + 1;

		auto defaultHandler = Api::OpcodeHandlerSingleton::get().client.getDefaultFunc();
		if (!defaultHandler)
		{
			std::stringstream ss;
			ss << "iSerialNum:" << itemPtr->iSerialNum << "|type:" << (uint32_t)itemPtr->type << "|iOpcode:" << itemPtr->iOpcode << "|unregister";
			ASYNC_PIE_LOG("handlePBForward/handlePBForward", PIE_CYCLE_HOUR, PIE_ERROR, "%s", ss.str().c_str());
			return;
		}

		defaultHandler(itemPtr->iSerialNum, itemPtr->iOpcode, itemPtr->sMsg);
		break;
	}
	default:
	{
		std::stringstream ss;
		ss << "iSerialNum:" << itemPtr->iSerialNum << "|type:" << (uint32_t)itemPtr->type << "|iOpcode:" << itemPtr->iOpcode << "| invalid type";
		ASYNC_PIE_LOG("handlePBForward/handlePBForward", PIE_CYCLE_HOUR, PIE_ERROR, "%s", ss.str().c_str());
		break;
	}
	}
}

void DispatcherImpl::handleSendData(SendData *itemPtr)
{
	switch (itemPtr->type)
	{
	case ConnetionType::CT_CLIENT:
	{
		auto ptrConnection = getClientConnection(itemPtr->iSerialNum);
		if (ptrConnection == nullptr)
		{
			return;
		}
		ptrConnection->handleSend(itemPtr->sData.data(), itemPtr->sData.size());
		break;
	}
	case ConnetionType::CT_SERVER:
	{
		auto ptrConnection = getConnection(itemPtr->iSerialNum);
		if (ptrConnection == nullptr)
		{
			return;
		}
		ptrConnection->handleSend(itemPtr->sData.data(), itemPtr->sData.size());
		break;
	}
	default:
		break;
	}
}

void DispatcherImpl::handleSendDataByFlag(SendDataByFlag *itemPtr)
{
	switch (itemPtr->type)
	{
	case ConnetionType::CT_CLIENT:
	{
		auto ptrConnection = getClientConnection(itemPtr->iSerialNum);
		if (ptrConnection == nullptr)
		{
			return;
		}

		//先压缩后加密
		std::string sBody = itemPtr->sBody;
		if (itemPtr->head.iFlags & PH_COMPRESSED)
		{
			Compressor::LZ4CompressorImpl compressor;
			auto optData = compressor.compress(sBody, 0);
			if (!optData.has_value())
			{
				return;
			}

			auto iBodyLen = optData.value().size();
			itemPtr->head.iBodyLen = iBodyLen;

			sBody = optData.value();
		}

		if (itemPtr->head.iFlags & PH_CRYPTO)
		{
			if (ptrConnection->getSessionKey().has_value())
			{
				sBody = APie::Crypto::Utility::encode_rc4(ptrConnection->getSessionKey().value(), sBody);
			}
			else
			{
				uint8_t iFlag = ~PH_CRYPTO;
				itemPtr->head.iFlags = itemPtr->head.iFlags & iFlag;
			}
		}

		std::string sData;
		sData.append(reinterpret_cast<char*>(&itemPtr->head), sizeof(ProtocolHead));
		sData.append(sBody);

		ptrConnection->handleSend(sData.data(), sData.size());
		break;
	}
	case ConnetionType::CT_SERVER:
	{
		auto ptrConnection = getConnection(itemPtr->iSerialNum);
		if (ptrConnection == nullptr)
		{
			return;
		}

		// 先压缩后加密
		std::string sBody = itemPtr->sBody;
		if (itemPtr->head.iFlags & PH_COMPRESSED)
		{
			Compressor::LZ4CompressorImpl compressor;
			auto optData = compressor.compress(sBody, 0);
			if (!optData.has_value())
			{
				return;
			}

			auto iBodyLen = optData.value().size();
			itemPtr->head.iBodyLen = iBodyLen;

			sBody = optData.value();
		}

		if (itemPtr->head.iFlags & PH_CRYPTO)
		{
			if (ptrConnection->getSessionKey().has_value())
			{
				sBody = APie::Crypto::Utility::encode_rc4(ptrConnection->getSessionKey().value(), sBody);
			}
			else
			{
				uint8_t iFlag = ~PH_CRYPTO;
				itemPtr->head.iFlags = itemPtr->head.iFlags & iFlag;
			}
		}

		std::string sData;
		sData.append(reinterpret_cast<char*>(&itemPtr->head), sizeof(ProtocolHead));
		sData.append(sBody);

		ptrConnection->handleSend(sData.data(), sData.size());
		break;
	}
	default:
		break;
	}
}


void DispatcherImpl::handleDial(DialParameters* ptrCmd)
{
	ClientConnection::createClient(this->tid_, &(this->base()), ptrCmd);
}

void DispatcherImpl::handleDialResult(DialResult* ptrCmd)
{
	auto clientProxy = APie::ClientProxy::findClientProxy(ptrCmd->iSerialNum);
	if (clientProxy)
	{
		clientProxy->setLocalIp(ptrCmd->sLocalIp);
		clientProxy->onConnect(ptrCmd->iResult);
	}
	else
	{
		ASYNC_PIE_LOG("DispatcherImpl/handleDialResult", PIE_CYCLE_HOUR, PIE_ERROR, "iSerialNum:%lld|iResult:%d|sLocalIp:%s", 
			ptrCmd->iSerialNum, ptrCmd->iResult, ptrCmd->sLocalIp.c_str());
	}
}

void DispatcherImpl::handleSetServerSessionAttr(SetServerSessionAttr* ptrCmd)
{
	auto ptrServer = APie::Event::DispatcherImpl::getConnection(ptrCmd->iSerialNum);
	if (ptrServer != nullptr)
	{
		ptrServer->handleSetServerSessionAttr(ptrCmd);
	}
	else
	{
		std::stringstream ss;
		ss << "null|iSerialNum:" << ptrCmd->iSerialNum;
		ASYNC_PIE_LOG("DispatcherImpl/handleSetServerSessionAttr", PIE_CYCLE_HOUR, PIE_ERROR, ss.str().c_str());
	}
}

void DispatcherImpl::handleSetClientSessionAttr(SetClientSessionAttr* ptrCmd)
{
	auto ptrClient = APie::Event::DispatcherImpl::getClientConnection(ptrCmd->iSerialNum);
	if (ptrClient != nullptr)
	{
		ptrClient->handleSetClientSessionAttr(ptrCmd);
	}
	else
	{
		std::stringstream ss;
		ss << "null|iSerialNum:" << ptrCmd->iSerialNum;
		ASYNC_PIE_LOG("DispatcherImpl/handleSetClientSessionAttr", PIE_CYCLE_HOUR, PIE_ERROR, ss.str().c_str());
	}
}

void DispatcherImpl::handleLogicCmd(LogicCmd* ptrCmd)
{
	std::string cmd = ptrCmd->sCmd;

	::pubsub::LOGIC_CMD msg;

	std::vector<std::string> fields = APie::SplitString(cmd, "|", APie::TRIM_WHITESPACE, APie::SPLIT_WANT_ALL);
	auto firstIte = fields.begin();
	if (firstIte != fields.end())
	{
		msg.set_cmd(*firstIte);
		fields.erase(firstIte);
	}

	for (const auto& items : fields)
	{
		auto ptrParams = msg.add_params();
		*ptrParams = items;
	}

	PubSubSingleton::get().publish(::pubsub::PUB_TOPIC::PT_LogicCmd, msg);
}

void DispatcherImpl::handleAsyncCallFunctor(LogicAsyncCallFunctor* ptrCmd)
{
	ptrCmd->callFunctor();
}

void DispatcherImpl::handleClientPeerClose(ClientPeerClose* ptrCmd)
{
	::pubsub::CLIENT_PEER_CLOSE msg;
	msg.set_serial_num(ptrCmd->iSerialNum);
	msg.set_result(ptrCmd->iResult);
	msg.set_info(ptrCmd->sInfo);
	msg.set_active(ptrCmd->iActive);

	PubSubSingleton::get().publish(::pubsub::PUB_TOPIC::PT_ClientPeerClose, msg);
}

void DispatcherImpl::handleCloseLocalClient(CloseLocalClient* ptrCmd)
{
	APie::Event::DispatcherImpl::delClientConnection(ptrCmd->iSerialNum);
}

void DispatcherImpl::handleCloseLocalServer(CloseLocalServer* ptrCmd)
{
	auto ptrServer = APie::Event::DispatcherImpl::getConnection(ptrCmd->iSerialNum);
	if (ptrServer != nullptr)
	{
		std::stringstream ss;
		ss << "active close|iSerialNum:" << ptrCmd->iSerialNum
			<< ",address:" << ptrServer->ip() << "->" << ptrServer->peerIp();
		ASYNC_PIE_LOG("DispatcherImpl/handleCloseLocalServer", PIE_CYCLE_HOUR, PIE_NOTICE, ss.str().c_str());
	}
	APie::Event::DispatcherImpl::delConnection(ptrCmd->iSerialNum);
}

void DispatcherImpl::handleServerPeerClose(ServerPeerClose* ptrCmd)
{
	::pubsub::SERVER_PEER_CLOSE msg;
	msg.set_serial_num(ptrCmd->iSerialNum);
	msg.set_result(ptrCmd->iResult);
	msg.set_info(ptrCmd->sInfo);
	msg.set_active(ptrCmd->iActive);

	PubSubSingleton::get().publish(::pubsub::PUB_TOPIC::PT_ServerPeerClose, msg);
}

void DispatcherImpl::handleLogicStart(uint32_t iThreadId)
{
	if (iThreadId != this->tid_)
	{
		return;
	}

	try {
		CtxSingleton::get().getEndpoint()->init();
		APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Start);
	}
	catch (YAML::InvalidNode& e) {
		std::stringstream ss;
		ss << "InvalidNode exception: " << e.what();

		PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "Exception", ss.str().c_str());
		throw;
	}
	catch (std::exception& e) {
		std::stringstream ss;
		ss << "Unexpected exception: " << e.what();

		PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "Exception", ss.str().c_str());
		throw;
	}
}

void DispatcherImpl::handleLogicExit(uint32_t iThreadId)
{
	if (iThreadId != this->tid_)
	{
		return;
	}

	APie::Hook::HookRegistrySingleton::get().triggerHook(Hook::HookPoint::HP_Exit);

	terminating_ = true;
}

void DispatcherImpl::handleStopThread(uint32_t iThreadId)
{
	if (iThreadId != this->tid_)
	{
		return;
	}

	this->exit();
}

void DispatcherImpl::handleRotate(time_t cutTime)
{
	if (cutTime < i_next_check_rotate)
	{
		return;
	}
	i_next_check_rotate = cutTime + 120;

	checkRotate();
}

void DispatcherImpl::handleAsyncLog(LogCmd* ptrCmd)
{
	pieLogRaw(ptrCmd->sFile.c_str(), ptrCmd->iCycle, ptrCmd->iLevel, ptrCmd->sMsg.c_str(), ptrCmd->bIgnoreMore);
}

void DispatcherImpl::handleMetric(MetricData* ptrCmd)
{
	auto metric = influxdb_cpp::builder();
	auto& measure = metric.meas(ptrCmd->sMetric);

	std::size_t iTagCount = ptrCmd->tag.size();
	std::size_t iCurTag = 0;
	for (const auto& items : ptrCmd->tag)
	{
		iCurTag++;
		if (iCurTag < iTagCount)
		{
			measure.tag(items.first, items.second);
		}
		else
		{
			influxdb_cpp::detail::field_caller *ptrField = nullptr;

			std::size_t iCount = ptrCmd->field.size();
			std::size_t iIndex = 0;
			for (const auto& elems : ptrCmd->field)
			{
				iIndex++;
				if (iIndex == 1)
				{
					ptrField = &(measure.tag(items.first, items.second).field(elems.first, elems.second));
				}
				else if (iIndex < iCount)
				{
					if (ptrField != nullptr)
					{
						ptrField->field(elems.first, elems.second);
					}
				}
				else
				{
					std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
					auto duration = now.time_since_epoch();
					auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);

					//uint64_t iCurTime = time(NULL) * 1000000000;
					uint64_t iCurTime = nanoseconds.count();

					std::string ip = APie::CtxSingleton::get().yamlAs<std::string>({ "metrics","ip" }, "127.0.0.1");
					uint16_t port = APie::CtxSingleton::get().yamlAs<uint16_t>({ "metrics","udp_port" }, 8089);

					if (ptrField != nullptr)
					{
						ptrField->field(elems.first, elems.second).timestamp(iCurTime).send_udp(ip, port);
					}
				}
			}
		}
	}
}

void DispatcherImpl::registerEndpoint()
{
}

} // namespace Event
} // namespace Envoy
