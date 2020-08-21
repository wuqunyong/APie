#include "os_sys_calls.h"

#include <chrono>
#include <string>

#include "../network/windows_platform.h"

namespace APie {
namespace Api {

uint32_t OsSysCalls::getCurProcessId()
{
#ifdef WIN32
	uint32_t pid = (uint32_t)GetCurrentProcessId();
	return pid;
#else
	uint32_t pid = (uint32_t)getpid();
	return pid;
#endif
}

} // namespace Api
} // namespace APie
