#pragma once

#include <cstdint>

#include "../event/file_event.h"

#include "../event/dispatcher_impl.h"
#include "../event/event_impl_base.h"

namespace APie {
namespace Event {

/**
 * Implementation of FileEvent for libevent that uses persistent events and
 * assumes the user will read/write until EAGAIN is returned from the file.
 */
class FileEventImpl : public FileEvent, ImplBase {
public:
  FileEventImpl(DispatcherImpl& dispatcher, int fd, FileReadyCb cb, FileTriggerType trigger,
                uint32_t events);

  // Event::FileEvent
  void activate(uint32_t events) override;
  void setEnabled(uint32_t events) override;

private:
  void assignEvents(uint32_t events);

  FileReadyCb cb_;
  event_base* base_;
  int fd_;
  FileTriggerType trigger_;
};

} // namespace Event
} // namespace Envoy
