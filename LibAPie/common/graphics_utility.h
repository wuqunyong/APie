#pragma once

#include <chrono>
#include <cstdint>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace APie {
namespace Common {

	struct xyBresenham
	{
		int32_t x;
		int32_t y;
	};

	// Bresenham Line Algorithm
	extern std::vector<xyBresenham> BresenhamLine(int x1, int y1, int x2, int y2);
}
} // namespace APie
