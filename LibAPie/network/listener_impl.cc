#include "../network/listener_impl.h"

#include <string>
#include <sstream>
#include <iostream>

#include "../common/empty_string.h"
#include "../event/dispatcher_impl.h"
#include "../event/file_event_impl.h"

#include "event2/listener.h"

namespace Envoy {
namespace Network {

void ListenerImpl::listenCallback(evconnlistener*, evutil_socket_t fd, sockaddr* remote_addr, int remote_addr_len, void* arg) {
  ListenerImpl* listener = static_cast<ListenerImpl*>(arg);
  listener->cb_->onAccept(fd);
}

void ListenerImpl::errorCallback(evconnlistener* listener, void* context) {
	int err = EVUTIL_SOCKET_ERROR();

	std::stringstream ss;
	ss << "Got an error %d (%s) on the listener. Shutting down.\n", err, evutil_socket_error_to_string(err);
	std::cout << ss.str().c_str() << std::endl;
}


ListenerImpl::ListenerImpl(Event::DispatcherImpl& dispatcher, ListenerCbPtr cb, uint16_t port, int backlog) :
	dispatcher_(dispatcher),
	cb_(cb),
	listener_(nullptr) {
	setupServerSocket(dispatcher, port, backlog);
}

ListenerImpl::~ListenerImpl()
{

}

void ListenerImpl::setupServerSocket(Event::DispatcherImpl& dispatcher, uint16_t port, int backlog) {
	memset(&listener_add_, 0, sizeof(listener_add_));
	listener_add_.sin_family = AF_INET;
	listener_add_.sin_addr.s_addr = INADDR_ANY;
	listener_add_.sin_port = htons(port);

	auto ptrListen = evconnlistener_new_bind(&dispatcher.base(), listenCallback, (void *)this,
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, backlog,
		(struct sockaddr*)&listener_add_,
		sizeof(listener_add_));

	listener_.reset(ptrListen);

	if (!listener_)
	{
		std::stringstream ss;
		ss << "cannot listen on socket: port:" << port;
		throw CreateListenerException(ss.str());
	}

	evconnlistener_set_error_cb(listener_.get(), errorCallback);
}

void ListenerImpl::enable() {
  if (listener_.get()) {
    evconnlistener_enable(listener_.get());
  }
}

void ListenerImpl::disable() {
  if (listener_.get()) {
    evconnlistener_disable(listener_.get());
  }
}

} // namespace Network
} // namespace Envoy
