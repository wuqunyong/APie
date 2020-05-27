#pragma once

#include <chrono>
#include <cstdint>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>


#include "../common/time.h"


namespace APie {

static const std::string EMPTY_STRING = "";


/**
 * Real-world time implementation of TimeSource.
 */
class RealTimeSource : public TimeSource {
public:
  // TimeSource
  SystemTime systemTime() override { return std::chrono::system_clock::now(); }
  MonotonicTime monotonicTime() override { return std::chrono::steady_clock::now(); }
};

} // namespace Envoy
