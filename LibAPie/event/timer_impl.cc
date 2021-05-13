#include "../event/timer_impl.h"

#include <chrono>

#include "event2/event.h"

#include "../network/ctx.h"

namespace APie {
namespace Event {

	std::atomic<uint32_t> TimerImpl::s_callCount = 0;

TimerImpl::TimerImpl(Libevent::BasePtr& libevent, TimerCb cb) : cb_(cb) {
  evtimer_assign(
      &raw_event_, libevent.get(),
	  [](evutil_socket_t, short, void* arg) -> void { static_cast<TimerImpl*>(arg)->cb_();  s_callCount++; }, this);
}

void TimerImpl::disableTimer() { event_del(&raw_event_); }

void TimerImpl::enableTimer(const std::chrono::milliseconds& d) {
  if (d.count() == 0) {
    event_active(&raw_event_, EV_TIMEOUT, 0);
  } else {
    // TODO(#4332): use duration_cast more nicely to clean up this code.
    std::chrono::microseconds us = std::chrono::duration_cast<std::chrono::microseconds>(d);
    timeval tv;
    tv.tv_sec = (long)(us.count() / 1000000);
    tv.tv_usec = us.count() % 1000000;
    event_add(&raw_event_, &tv);
  }
}

bool TimerImpl::enabled() { return 0 != evtimer_pending(&raw_event_, nullptr); }


void EphemeralTimer::enableTimer(uint64_t interval)
{
	m_timer->enableTimer(std::chrono::milliseconds(interval));
}

std::shared_ptr<EphemeralTimer> EphemeralTimerMgr::createEphemeralTimer(TimerCb cb)
{
	m_id++;

	uint64_t iId = m_id;
	auto timerCb = [iId, cb, this]() {
		cb();
		this->deleteEphemeralTimer(iId);
	};
	auto ptrTimer = std::make_shared<EphemeralTimer>();
	ptrTimer->m_timer = APie::CtxSingleton::get().getLogicThread()->dispatcher().createTimer(timerCb);
	ptrTimer->m_id = iId;

	m_ephemeralCache[iId] = ptrTimer;

	return ptrTimer;
}

void EphemeralTimerMgr::deleteEphemeralTimer(uint64_t id)
{
	m_ephemeralCache.erase(id);
}

} // namespace Event
} // namespace Envoy
