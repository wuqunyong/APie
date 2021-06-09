#pragma once

#include <chrono>
#include <cstdint>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include <tuple>
#include <type_traits>

#include "../common/time.h"


namespace APie {
namespace Common {

/**
 * Real-world time implementation of TimeSource.
 */
class RealTimeSource : public TimeSource {
public:
  // TimeSource
	SystemTime systemTime() override;
	MonotonicTime monotonicTime() override;
};


template <typename T>
struct func_traits : func_traits<decltype(&T::operator())> {};

template <typename C, typename R, typename... Args>
struct func_traits<R(C::*)(Args...)> : func_traits<R(*)(Args...)> {};

template <typename C, typename R, typename... Args>
struct func_traits<R(C::*)(Args...) const> : func_traits<R(*)(Args...)> {};

template <typename R, typename... Args> struct func_traits<R(*)(Args...)> {
	using result_type = R;
	using arg_count = std::integral_constant<std::size_t, sizeof...(Args)>;
	using args_type = std::tuple<typename std::decay<Args>::type...>;
};


}
} // namespace APie
