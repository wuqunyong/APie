#include "../api/api_impl.h"

#include <chrono>
#include <string>

#include "../event/dispatcher_impl.h"

namespace APie {
namespace Api {

Impl::Impl()
{
}

Event::DispatcherPtr Impl::allocateDispatcher(Event::EThreadType type, uint32_t tid) {
  return std::make_unique<Event::DispatcherImpl>(type, tid);
}


} // namespace Api
} // namespace Envoy
