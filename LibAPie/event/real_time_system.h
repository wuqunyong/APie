#pragma once

#include "../event/timer.h"
#include "../common/utility.h"

namespace APie {
namespace Event {

/**
 * Real-world time implementation of TimeSystem.
 */
class RealTimeSystem : public TimeSystem {
public:
  // TimeSystem
  SchedulerPtr createScheduler(Scheduler&) override;

  // TimeSource
  SystemTime systemTime() override { return time_source_.systemTime(); }
  MonotonicTime monotonicTime() override { return time_source_.monotonicTime(); }

private:
  RealTimeSource time_source_;
};

} // namespace Event
} // namespace Envoy
