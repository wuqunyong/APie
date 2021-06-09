#include "../event/real_time_system.h"

#include <chrono>

#include "../event/timer_impl.h"

namespace APie {
namespace Event {
namespace {

class RealScheduler : public Scheduler {
public:
  RealScheduler(Scheduler& base_scheduler) : base_scheduler_(base_scheduler) {}
  TimerPtr createTimer(const TimerCb& cb) override { return base_scheduler_.createTimer(cb); };

private:
  Scheduler& base_scheduler_;
};

} // namespace

SchedulerPtr RealTimeSystem::createScheduler(Scheduler& base_scheduler) {
  return std::make_unique<RealScheduler>(base_scheduler);
}

} // namespace Event
} // namespace Envoy
