#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../common/time.h"
#include "../event/file_event.h"
#include "../event/signal.h"
#include "../event/timer.h"
#include "../event/deferred_deletable.h"

#include "../network/listener.h"
#include "../network/command.h"


namespace APie {
namespace Event {

	enum class EThreadType
	{
		TT_None = 0,
		TT_Listen,
		TT_IO,
		TT_Logic,
		TT_Log,
	};

/**
 * Callback invoked when a dispatcher post() runs.
 */
typedef std::function<void()> PostCb;

/**
 * Abstract event dispatching loop.
 */
class Dispatcher {
public:
  virtual ~Dispatcher() {}


  virtual void clearDeferredDeleteList() PURE;

  virtual Network::ListenerPtr createListener(Network::ListenerCbPtr cb, Network::ListenerConfig config) PURE;

  virtual Event::TimerPtr createTimer(TimerCb cb) PURE;

  virtual void deferredDelete(DeferredDeletablePtr&& to_delete) PURE;

  virtual void start() PURE;

  virtual void exit() PURE;

  virtual SignalEventPtr listenForSignal(int signal_num, SignalCb cb) PURE;

  virtual void post(PostCb callback) PURE;

  virtual void run(void) PURE;

  virtual void push(Command& cmd) PURE;

  virtual std::atomic<bool>& terminating() PURE;
};

typedef std::unique_ptr<Dispatcher> DispatcherPtr;

} // namespace Event
} // namespace APie
