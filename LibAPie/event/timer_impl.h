#pragma once

#include <chrono>

#include "../event/timer.h"

#include "../event/event_impl_base.h"
#include "../event/libevent.h"

namespace APie {
namespace Event {

/**
 * libevent implementation of Timer.
 */
class TimerImpl : public Timer, ImplBase {
public:
  TimerImpl(Libevent::BasePtr& libevent, TimerCb cb);

  // Timer
  void disableTimer() override;
  void enableTimer(const std::chrono::milliseconds& d) override;
  bool enabled() override;

private:
  TimerCb cb_;
};

} // namespace Event
} // namespace Envoy
