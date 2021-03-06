#include "libevent.h"

#include <signal.h>

#include "event2/thread.h"

namespace APie {
namespace Event {
namespace Libevent {

bool Global::initialized_ = false;

void Global::initialize() {
#ifdef WIN32
  evthread_use_windows_threads();
#else
  evthread_use_pthreads();

  // Ignore SIGPIPE and allow errors to propagate through error codes.
  signal(SIGPIPE, SIG_IGN);
#endif
  initialized_ = true;
}

} // namespace Libevent
} // namespace Event
} // namespace Envoy
