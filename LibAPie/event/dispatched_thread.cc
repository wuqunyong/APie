#include "../event/dispatched_thread.h"

#include <chrono>
#include <memory>

#include "../common/time.h"
#include "../event/dispatcher.h"


namespace Envoy {
namespace Event {

void DispatchedThreadImpl::start(void) {

	thread_ = std::thread([this]() -> void { threadRoutine(); });
}

void DispatchedThreadImpl::exit() {
  if (thread_.joinable()) {
    dispatcher_->exit();
    thread_.join();
  }
}

void DispatchedThreadImpl::threadRoutine(void) 
{
	dispatcher_->start();
	dispatcher_->run();
	dispatcher_.reset();
}

} // namespace Event
} // namespace Envoy
