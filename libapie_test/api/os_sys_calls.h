#pragma once

#ifdef WIN32
#else
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <memory>
#include <string>

#include "../singleton/threadsafe_singleton.h"

namespace APie {
namespace Api {

class OsSysCalls {

public:
	uint32_t getCurProcessId();
};

typedef ThreadSafeSingleton<OsSysCalls> OsSysCallsSingleton;

} // namespace Api
} // namespace APie
