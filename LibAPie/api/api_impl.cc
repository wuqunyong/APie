#include "../api/api_impl.h"

#include <chrono>
#include <string>

#include "../event/dispatcher_impl.h"

namespace Envoy {
namespace Api {

Impl::Impl()
{
}

Event::DispatcherPtr Impl::allocateDispatcher(uint32_t tid) {
  return std::make_unique<Event::DispatcherImpl>(tid);
}


} // namespace Api
} // namespace Envoy
