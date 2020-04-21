#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <vector>
#include <mutex>

#include "../api/api.h"
#include "../common/time.h"
#include "../event/deferred_deletable.h"
#include "../event/dispatcher.h"
#include "../event/libevent.h"
#include "../event/libevent_scheduler.h"

namespace Envoy {
namespace Event {

/**
 * libevent implementation of Event::Dispatcher.
 */
class DispatcherImpl : public Dispatcher {
public:
  DispatcherImpl();
  ~DispatcherImpl();

  /**
   * @return event_base& the libevent base.
   */
  event_base& base() { return base_scheduler_.base(); }

  void clearDeferredDeleteList() override;
  Network::ListenerPtr createListener(Network::ListenerCallbacks& cb, uint16_t port, int backlog) override;
  TimerPtr createTimer(TimerCb cb) override;
  void deferredDelete(DeferredDeletablePtr&& to_delete) override;

  void exit() override;
  SignalEventPtr listenForSignal(int signal_num, SignalCb cb) override;
  void post(std::function<void()> callback) override;
  void run(RunType type) override;

private:
  void runPostCallbacks();


  LibeventScheduler base_scheduler_;
  TimerPtr deferred_delete_timer_;
  TimerPtr post_timer_;
  std::vector<DeferredDeletablePtr> to_delete_1_;
  std::vector<DeferredDeletablePtr> to_delete_2_;
  std::vector<DeferredDeletablePtr>* current_to_delete_;
  std::mutex post_lock_;
  //std::list<std::function<void()>> post_callbacks_ GUARDED_BY(post_lock_);
  std::list<std::function<void()>> post_callbacks_;
  bool deferred_deleting_{};
};

} // namespace Event
} // namespace Envoy
