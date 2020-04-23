#include "../event/dispatcher_impl.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <cassert>

#include "../api/api.h"
#include "../network/listener.h"


#include "../event/libevent_scheduler.h"
#include "../event/signal_impl.h"
#include "../event/timer_impl.h"
#include "../network/listener_impl.h"
#include "../network/connection.h"

#include "event2/event.h"
#include <assert.h>

namespace Envoy {
namespace Event {

	std::atomic<uint64_t> DispatcherImpl::serial_num_(0);

DispatcherImpl::DispatcherImpl()
	: deferred_delete_timer_(createTimer([this]() -> void { clearDeferredDeleteList(); })),
	post_timer_(createTimer([this]() -> void { runPostCallbacks(); })),
	current_to_delete_(&to_delete_1_) {}

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

Network::ListenerPtr DispatcherImpl::createListener(Network::ListenerCbPtr cb, uint16_t port, int backlog) {

  return Network::ListenerPtr{new Network::ListenerImpl(*this, cb, port, backlog)};
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
  base_scheduler_.run();
}

void DispatcherImpl::push(Command& cmd)
{
	mailbox_.send(cmd);
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
	size_t iLoopCount = mailbox_.size();

	while (iLoopCount >= 0)
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

static void readcb(struct bufferevent *bev, void *arg)
{
	Connection* ptrConnection = (Connection *)arg;
	ptrConnection->readcb();
}

static void writecb(struct bufferevent *bev, void *arg)
{
	Connection *ptrConnection = (Connection *)arg;
	ptrConnection->writecb();
}

static void eventcb(struct bufferevent *bev, short what, void *arg)
{
	Connection *ptrConnection = (Connection *)arg;
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
	auto ptrConnection = std::make_shared<Connection>(iSerialNum, bev, itemPtr->iType);
	if (nullptr == ptrConnection)
	{
		bufferevent_free(bev);
		return;
	}

	bufferevent_setcb(bev, readcb, writecb, eventcb, ptrConnection.get());
	bufferevent_enable(bev, EV_READ | EV_WRITE);
}

} // namespace Event
} // namespace Envoy
