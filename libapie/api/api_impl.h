#pragma once

#include <chrono>
#include <string>

#include "../api/api.h"
#include "../event/timer.h"

namespace APie {
namespace Api {

/**
 * Implementation of Api::Api
 */
class Impl : public Api {
public:
  Impl();

  // Api::Api
  Event::DispatcherPtr allocateDispatcher(Event::EThreadType type, uint32_t tid) override;
};

} // namespace Api
} // namespace Envoy
