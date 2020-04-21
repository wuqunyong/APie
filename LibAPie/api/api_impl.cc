#include "../api/api_impl.h"

#include <chrono>
#include <string>

#include "../event/dispatcher_impl.h"

namespace Envoy {
namespace Api {

Impl::Impl()
{
}

Event::DispatcherPtr Impl::allocateDispatcher() {
  return std::make_unique<Event::DispatcherImpl>();
}


} // namespace Api
} // namespace Envoy
