#include <new>
#include <string.h>
#include <assert.h>
#include <sstream>
#include <iostream>

#include "../network/Ctx.h"
#include "../network/address.h"

#ifdef WIN32
#define SLEEP_MS(ms) Sleep(ms)
#else
#define SLEEP_MS(ms) usleep((ms) * 1000)

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif



namespace Envoy {

	class PortCb : public Network::ListenerCallbacks
	{
		void onAccept(evutil_socket_t fd)
		{
			std::cout << fd << std::endl;

			auto ptrAddr = Network::addressFromFd(fd);
			if (ptrAddr != nullptr)
			{
				std::string addr = Network::makeFriendlyAddress(*ptrAddr);
			}
		}

	};

Ctx::Ctx()
{

}

Ctx::~Ctx()
{

}

void Ctx::init()
{

	ThreadVec listenerVec;
	auto ptrListen = std::make_shared<Event::DispatchedThreadImpl>();
	auto ptrCb = std::make_shared<PortCb>();
	ptrListen->push(ptrListen->dispatcher().createListener(ptrCb, 5007, 1024));
	ptrListen->start();

	listenerVec.push_back(ptrListen);
	thread_[Event::EThreadType::TT_Listen] = listenerVec;

}

void Ctx::destroy()
{

}

}
