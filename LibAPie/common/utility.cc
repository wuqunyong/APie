#include "../common/utility.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <string>


namespace APie {
namespace Common {

	SystemTime RealTimeSource::systemTime()
	{
		return std::chrono::system_clock::now();
	}

	MonotonicTime RealTimeSource::monotonicTime()
	{
		return std::chrono::steady_clock::now();
	}

}
} // namespace APie
