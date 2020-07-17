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
#include "../network/connection.h"
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
		ptrData->tag["thread_type"] = toStirng(type_) + "_" + std::to_string(tid_);
		ptrData->field["mailbox"] = (double)mailbox_.size();

		Command command;
		command.type = Command::metric_data;
		command.args.metric_data.ptrData = ptrData;

		auto ptrMetric = APie::CtxSingleton::get().getMetricsThread();
		if (ptrMetric != nullptr)
		{
			ptrMetric->push(command);;
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

	interval_timer_->enableTimer(std::chrono::milliseconds(200));
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
		case Command::send_data:
		{
			this->handleSendData(cmd.args.send_data.ptrData);
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
		case Command::logic_cmd:
		{
			this->handleLogicCmd(cmd.args.logic_cmd.ptrData);
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

	bufferevent_setcb(bev, readcb, writecb, eventcb, ptrConnection.get());
	bufferevent_enable(bev, EV_READ | EV_WRITE);

	DispatcherImpl::addConnection(ptrConnection);
}

void DispatcherImpl::handlePBRequest(PBRequest *itemPtr)
{
	switch (itemPtr->type)
	{
	case APie::ConnetionType::CT_SERVER:
	{
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


void DispatcherImpl::handleDial(DialParameters* ptrCmd)
{
	ClientConnection::createClient(this->tid_, &(this->base()), ptrCmd);
}

void DispatcherImpl::handleDialResult(DialResult* ptrCmd)
{
	auto clientProxy = APie::ClientProxy::findClient(ptrCmd->iSerialNum);
	if (clientProxy)
	{
		clientProxy->onConnect(ptrCmd->iResult);
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

		PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "fatalExit", ss.str().c_str());
		throw;
	}
	catch (std::exception& e) {
		std::stringstream ss;
		ss << "Unexpected exception: " << e.what();

		PIE_LOG("Exception/Exception", PIE_CYCLE_HOUR, PIE_ERROR, "%s: %s", "fatalExit", ss.str().c_str());
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
	pieLogRaw(ptrCmd->sFile.c_str(), ptrCmd->iCycle, ptrCmd->iLevel, ptrCmd->sMsg.c_str());
}

void DispatcherImpl::handleMetric(MetricData* ptrCmd)
{
	auto metric = influxdb_cpp::builder();
	auto& measure = metric.meas(ptrCmd->sMetric);
	for (const auto& items : ptrCmd->tag)
	{
		measure.tag(items.first, items.second);
	}
	std::size_t iCount = ptrCmd->field.size();
	std::size_t iIndex = 0;
	for (const auto& items : ptrCmd->field)
	{
		iIndex++;
		if (iIndex < iCount)
		{
			measure.field(items.first, items.second);
		}
		else
		{
			std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
			auto duration = now.time_since_epoch();
			auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);

			//uint64_t iCurTime = time(NULL) * 1000000000;
			uint64_t iCurTime = nanoseconds.count();

			std::string ip = APie::CtxSingleton::get().yamlAs<std::string>({"metrics","ip"}, "127.0.0.1");
			uint16_t port = APie::CtxSingleton::get().yamlAs<uint16_t>({"metrics","udp_port"}, 8089);
			measure.field(items.first, items.second).timestamp(iCurTime).send_udp(ip, port);
		}
	}
}

void DispatcherImpl::registerEndpoint()
{
}

} // namespace Event
} // namespace Envoy
