#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "../common/pure.h"
#include "../event/libevent.h"

#include <event2/util.h>

namespace Envoy {
namespace Network {

/**
 * Callbacks invoked by a listener.
 */
class ListenerCallbacks {
public:
  virtual ~ListenerCallbacks() {}

  virtual void onAccept(evutil_socket_t fd) PURE;

  virtual void onNewConnection(evutil_socket_t fd) PURE;
};


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
