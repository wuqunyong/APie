#pragma once

#include <chrono>
#include <memory>
#include <utility>
#include <thread>

#include "../api/api.h"
#include "../event/dispatcher.h"
#include "../event/timer.h"
#include "../event/dispatcher_impl.h"
#include "../network/listener.h"
#include "../network/Command.h"

namespace Envoy {
namespace Event {

/**
 * Generic dispatched thread.
 *
 * This provides basic functionality for a thread which has the "Dispatched
 * Nature" (runs an event loop) but does not want to do listening and accept new
 * connections like regular Worker threads, or any of the other functionality
 * specific to "Worker" threads. This is particularly useful if you need a
 * special purpose thread that will issue or receive gRPC calls.
 *
 * These features are set up:
 *   1) Dispatcher support:
 *      open connections, open files, callback posting, timers, listen
 *   2) GuardDog deadlock monitoring
 *
 * These features are not:
 *   1) Thread local storage (we don't want runOnAllThreads callbacks to run on
 *      this thread).
 *   2) ConnectionHandler and listeners
 *
 * TODO(dnoe): Worker should probably be refactored to leverage this.
 */

enum class EThreadType 
{ 
	TT_None = 0, 
	TT_Listen, 
	TT_IO,
	TT_Logic,
	TT_Log,
};

enum class DTState
{
	DTS_Ready = 0,
	DTS_Running,
	DTS_Exit,
};

class DispatchedThreadImpl  {
	
public:
  DispatchedThreadImpl(EThreadType type) :
	  type_(type),
	  state_(DTState::DTS_Ready),
	  dispatcher_(std::make_unique<Event::DispatcherImpl>())
  {
  }

  /**
   * Start the thread.
   *
   * @param guard_dog GuardDog instance to register with.
   */
  void start(void);
  DTState state();

  Dispatcher& dispatcher() { return *dispatcher_; }
  void push(std::shared_ptr<Network::Listener> listener);
  void push(Command& cmd);

  /**
   * Exit the dispatched thread. Will block until the thread joins.
   */
  void exit();

private:
  void threadRoutine(void);

  EThreadType type_;
  DTState state_;
  DispatcherPtr dispatcher_;
  std::thread thread_;
  std::vector<std::shared_ptr<Network::Listener>> listener_;
};

} // namespace Event
} // namespace Envoy
