#include "../event/dispatched_thread.h"

#include <chrono>
#include <memory>

#include "../common/time.h"
#include "../event/dispatcher.h"


namespace APie {
namespace Event {

void DispatchedThreadImpl::start(void) {
	state_ = DTState::DTS_Running;
	thread_ = std::thread([this]() -> void { threadRoutine(); });
}

DTState DispatchedThreadImpl::state()
{
	return state_;
}

uint32_t DispatchedThreadImpl::getTId()
{
	return tid_;
}

void DispatchedThreadImpl::exit() {
	state_ = DTState::DTS_Exit;
	if (thread_.joinable()) {
		dispatcher_->exit();
		thread_.join();
		listener_.clear();
	}
}

void DispatchedThreadImpl::push(std::shared_ptr<Network::Listener> listener)
{
	listener_.push_back(listener);
}

void DispatchedThreadImpl::push(Command& cmd)
{
	dispatcher_->push(cmd);
}

void DispatchedThreadImpl::threadRoutine(void) 
{
	dispatcher_->start();
	dispatcher_->run();
	dispatcher_.reset();
}

} // namespace Event
} // namespace Envoy
