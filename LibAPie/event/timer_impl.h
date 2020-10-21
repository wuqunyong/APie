#pragma once

#include <chrono>
#include <map>

#include "../event/timer.h"

#include "../event/event_impl_base.h"
#include "../event/libevent.h"
#include "../singleton/threadsafe_singleton.h"

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


class EphemeralTimer
{
public:
	void enableTimer(uint64_t interval);

private:
	uint64_t m_id = 0;
	Event::TimerPtr m_timer;

	friend class EphemeralTimerMgr;
};

class EphemeralTimerMgr
{
public:
	std::shared_ptr<EphemeralTimer> createEphemeralTimer(TimerCb cb);
	void deleteEphemeralTimer(uint64_t id);

public:
	uint64_t m_id = 0;
	std::map<uint64_t, std::shared_ptr<EphemeralTimer>> m_ephemeralCache;
};

using EphemeralTimerMgrSingleton = ThreadSafeSingleton<EphemeralTimerMgr>;

} // namespace Event
} // namespace Envoy
