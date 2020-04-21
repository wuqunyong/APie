#pragma once

#include <chrono>
#include <string>

#include "../api/api.h"
#include "../event/timer.h"

namespace Envoy {
namespace Api {

/**
 * Implementation of Api::Api
 */
class Impl : public Api {
public:
  Impl();

  // Api::Api
  Event::DispatcherPtr allocateDispatcher() override;
};

} // namespace Api
} // namespace Envoy
