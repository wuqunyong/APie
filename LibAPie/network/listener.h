#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "../common/pure.h"
#include "../event/libevent.h"

#include "i_poll_events.hpp"

#include <event2/util.h>


namespace APie {
namespace Network {

struct ListenerConfig
{
	std::string ip;
	uint16_t port = 0;
	ProtocolType type = ProtocolType::PT_None;
	int backlog = 1024;
};

/**
 * Callbacks invoked by a listener.
 */
class ListenerCallbacks {
public:
  virtual ~ListenerCallbacks() {}

  virtual void onAccept(evutil_socket_t fd) PURE;
};

typedef std::shared_ptr<ListenerCallbacks> ListenerCbPtr;

/**
 * An abstract socket listener. Free the listener to stop listening on the socket.
 */
class Listener {
public:
  virtual ~Listener() {}

  /**
   * Temporarily disable accepting new connections.
   */
  virtual void disable() PURE;

  /**
   * Enable accepting new connections.
   */
  virtual void enable() PURE;
};

typedef std::unique_ptr<Listener> ListenerPtr;


class CreateListenerException : public std::runtime_error {
public:
	CreateListenerException(const std::string& message) : std::runtime_error(message) {}
};


} // namespace Network
} // namespace Envoy
