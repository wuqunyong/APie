#pragma once

#include <memory>
#include <string>

#include "../common/time.h"
#include "../event/dispatcher.h"

namespace APie {
namespace Api {

/**
 * "Public" API that different components use to interact with the various system abstractions.
 */
class Api {
public:
  virtual ~Api() {}

  /**
   * Allocate a dispatcher.
   * @return Event::DispatcherPtr which is owned by the caller.
   */
  virtual Event::DispatcherPtr allocateDispatcher(Event::EThreadType type, uint32_t tid) PURE;


};

typedef std::unique_ptr<Api> ApiPtr;

} // namespace Api
} // namespace Envoy
