#pragma once

#include "../event/dispatcher.h"
#include "../event/timer.h"

#include "../event/libevent.h"

#include "event2/event.h"

namespace APie {
namespace Event {

// Implements Scheduler based on libevent.
class LibeventScheduler : public Scheduler {
public:
  LibeventScheduler();

  // Scheduler
  TimerPtr createTimer(const TimerCb& cb) override;

  /**
   * Runs the event loop.
   *
   * @param mode The mode in which to run the event loop.
   */
  void run(void);

  /**
   * Exits the libevent loop.
   */
  void loopExit();

  /**
   * TODO(jmarantz): consider strengthening this abstraction and instead of
   * exposing the libevent base pointer, provide API abstractions for the calls
   * into it. Among other benefits this might make it more tractable to someday
   * consider an alternative to libevent if the need arises.
   *
   * @return the underlying libevent structure.
   */
  event_base& base() { return *libevent_; }

private:
  Libevent::BasePtr libevent_;
};

} // namespace Event
} // namespace Envoy
