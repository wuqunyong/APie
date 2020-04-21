#include "../event/libevent_scheduler.h"

#include <assert.h>

#include "../event/timer_impl.h"

namespace Envoy {
namespace Event {

LibeventScheduler::LibeventScheduler() : libevent_(event_base_new()) {
  // The dispatcher won't work as expected if libevent hasn't been configured to use threads.
	assert(Libevent::Global::initialized());
}

TimerPtr LibeventScheduler::createTimer(const TimerCb& cb) {
  return std::make_unique<TimerImpl>(libevent_, cb);
};

void LibeventScheduler::run(Dispatcher::RunType mode) {
  event_base_dispatch(libevent_.get());
}

void LibeventScheduler::loopExit() { event_base_loopexit(libevent_.get(), nullptr); }

} // namespace Event
} // namespace Envoy
