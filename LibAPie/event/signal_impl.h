#pragma once

#include "../event/signal.h"

#include "../event/dispatcher_impl.h"
#include "../event/event_impl_base.h"

namespace APie {
namespace Event {

/**
 * libevent implementation of Event::SignalEvent.
 */
class SignalEventImpl : public SignalEvent, ImplBase {
public:
  SignalEventImpl(DispatcherImpl& dispatcher, int signal_num, SignalCb cb);

private:
  SignalCb cb_;
};

} // namespace Event
} // namespace Envoy
