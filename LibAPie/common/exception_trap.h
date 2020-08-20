#pragma once

#ifdef WIN32
#include "../network/windows_platform.h"
#else
#include <string>
#include <cstddef>
#include <sstream>

#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>
#include <execinfo.h>
#include <dirent.h>
#include <libgen.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif


namespace APie
{
	void ExceptionTrap();
}  // namespace APie
