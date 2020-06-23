#pragma once

#include "../network/listener.h"
#include "../event/dispatcher_impl.h"

namespace APie {
namespace Network {

/**
 * libevent implementation of Network::Listener for TCP.
 */
class ListenerImpl : public Listener {
public:
  ListenerImpl(Event::DispatcherImpl& dispatcher, ListenerCbPtr cb, Network::ListenerConfig config);
  ~ListenerImpl();

  void disable() override;
  void enable() override;

protected:
  void setupServerSocket(Event::DispatcherImpl& dispatcher);

  Event::DispatcherImpl& dispatcher_;
  ListenerCbPtr cb_;
  Network::ListenerConfig config_;

private:
  static void listenCallback(evconnlistener*, evutil_socket_t fd, sockaddr* remote_addr, int remote_addr_len, void* arg);
  static void errorCallback(evconnlistener* listener, void* context);

  struct sockaddr_in listener_add_;
  Event::Libevent::ListenerPtr listener_;
};

} // namespace Network
} // namespace Envoy
