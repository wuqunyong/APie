#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

#include "apie.h"

#include "service_init.h"


int main(int argc, char **argv)
{
	if (argc != 2)
	{
		PANIC_ABORT("usage: exe <ConfFile>, Expected: %d, got: %d", 2, argc);
	}

	std::string configFile = argv[1];

	APie::Hook::HookRegistrySingleton::get().registerHook(APie::Hook::HookPoint::HP_Init, APie::initHook);
	APie::Hook::HookRegistrySingleton::get().registerHook(APie::Hook::HookPoint::HP_Start, APie::startHook);
	APie::Hook::HookRegistrySingleton::get().registerHook(APie::Hook::HookPoint::HP_Ready, APie::readyHook);
	APie::Hook::HookRegistrySingleton::get().registerHook(APie::Hook::HookPoint::HP_Exit, APie::exitHook);

	APie::CtxSingleton::get().init(configFile);
	APie::CtxSingleton::get().start();
	APie::CtxSingleton::get().waitForShutdown();

    return 0;
}
