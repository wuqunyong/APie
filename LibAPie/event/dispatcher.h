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
#include "../network/Command.h"


namespace Envoy {
namespace Event {

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


  /**
   * Clear any items in the deferred deletion queue.
   */
  virtual void clearDeferredDeleteList() PURE;

  

  /**
   * Create a listener on a specific port.
   * @param socket supplies the socket to listen on.
   * @param cb supplies the callbacks to invoke for listener events.
   * @param bind_to_port controls whether the listener binds to a transport port or not.
   * @param hand_off_restored_destination_connections controls whether the listener searches for
   *        another listener after restoring the destination address of a new connection.
   * @return Network::ListenerPtr a new listener that is owned by the caller.
   */
  virtual Network::ListenerPtr createListener(Network::ListenerCbPtr cb, uint16_t port, int backlog) PURE;

  /**
   * Allocate a timer. @see Timer for docs on how to use the timer.
   * @param cb supplies the callback to invoke when the timer fires.
   */
  virtual Event::TimerPtr createTimer(TimerCb cb) PURE;

  /**
   * Submit an item for deferred delete. @see DeferredDeletable.
   */
  virtual void deferredDelete(DeferredDeletablePtr&& to_delete) PURE;

  virtual void start() PURE;

  /**
   * Exit the event loop.
   */
  virtual void exit() PURE;

  /**
   * Listen for a signal event. Only a single dispatcher in the process can listen for signals.
   * If more than one dispatcher calls this routine in the process the behavior is undefined.
   *
   * @param signal_num supplies the signal to listen on.
   * @param cb supplies the callback to invoke when the signal fires.
   * @return SignalEventPtr a signal event that is owned by the caller.
   */
  virtual SignalEventPtr listenForSignal(int signal_num, SignalCb cb) PURE;

  /**
   * Post a functor to the dispatcher. This is safe cross thread. The functor runs in the context
   * of the dispatcher event loop which may be on a different thread than the caller.
   */
  virtual void post(PostCb callback) PURE;


  virtual void run(void) PURE;

  virtual void push(Command& cmd) PURE;

};

typedef std::unique_ptr<Dispatcher> DispatcherPtr;

} // namespace Event
} // namespace Envoy
