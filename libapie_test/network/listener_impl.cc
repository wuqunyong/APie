#include "../network/listener_impl.h"

#include <string>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include "../common/empty_string.h"
#include "../event/dispatcher_impl.h"
#include "../event/file_event_impl.h"

#include "event2/listener.h"
#include "logger.h"

namespace APie {
namespace Network {

void ListenerImpl::listenCallback(evconnlistener*, evutil_socket_t fd, sockaddr* remote_addr, int remote_addr_len, void* arg) {
  ListenerImpl* listener = static_cast<ListenerImpl*>(arg);
  listener->cb_->onAccept(fd);
}

void ListenerImpl::errorCallback(evconnlistener* listener, void* context) {
	int err = EVUTIL_SOCKET_ERROR();

	std::stringstream ss;
	ss << "Got an error %d (%s) on the listener. Shutting down.\n", err, evutil_socket_error_to_string(err);
	ASYNC_PIE_LOG("ListenerImpl/errorCallback", PIE_CYCLE_DAY, PIE_ERROR, ss.str().c_str());

	ss.str("");
	ss << "listener accept failure|err:" << err << "|info:" << evutil_socket_error_to_string(err);
	PANIC_ABORT(ss.str().c_str());
}


ListenerImpl::ListenerImpl(Event::DispatcherImpl& dispatcher, ListenerCbPtr cb, Network::ListenerConfig config) :
	dispatcher_(dispatcher),
	cb_(cb),
	config_(config),
	listener_(nullptr) {
	setupServerSocket(dispatcher);
}

ListenerImpl::~ListenerImpl()
{

}

void ListenerImpl::setupServerSocket(Event::DispatcherImpl& dispatcher) {
	memset(&listener_add_, 0, sizeof(listener_add_));
	listener_add_.sin_family = AF_INET;
	
	if (config_.ip.compare("*") == 0)
	{
		listener_add_.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		int result = inet_pton(AF_INET, config_.ip.c_str(), &listener_add_.sin_addr);
		if (result != 1) 
		{
			std::stringstream ss;
			ss << "occur error using inet_pton with " << config_.ip << "|result:" << result;
			throw CreateListenerException(ss.str());
		}
	}
	
	listener_add_.sin_port = htons(config_.port);

	auto ptrListen = evconnlistener_new_bind(&dispatcher.base(), listenCallback, (void *)this,
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, config_.backlog,
		(struct sockaddr*)&listener_add_,
		sizeof(listener_add_));

	listener_.reset(ptrListen);

	if (!listener_)
	{
		std::stringstream ss;
		ss << "cannot listen on socket: port:" << config_.port;
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
