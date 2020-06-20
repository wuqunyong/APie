#pragma once

#ifdef WIN32
#include "../network/windows_platform.h"
#else
#include <unistd.h>
#include <pthread.h>
#endif


namespace APie
{
	void ExceptionTrap();
}  // namespace APie
