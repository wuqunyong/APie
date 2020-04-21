#include "network/windows_platform.h"

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

#include <windows.h>
#include <direct.h>

#include "yaml-cpp/yaml.h"

#include "event/real_time_system.h"
#include "event/dispatched_thread.h"
#include "event/libevent_scheduler.h"

#include "api/api_impl.h"
#include "network/platform_impl.h"

using namespace Envoy;

class PortCb : public Network::ListenerCallbacks
{
	void onAccept(evutil_socket_t fd)
	{
		std::cout << fd << std::endl;
	}

};

int main(int argc, char **argv)
{
	Event::Libevent::Global::initialize();
	PlatformImpl platform;

	PortCb cb;
	Event::DispatchedThreadImpl test1;
	auto ptrListen = test1.dispatcher().createListener(cb, 5007, 1024);
	test1.start();

	std::cin.get();

    return 0;
}
