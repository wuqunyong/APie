#pragma once

#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

namespace APie {

std::tuple<uint32_t, std::string> initHook();
std::tuple<uint32_t, std::string> startHook();
std::tuple<uint32_t, std::string> exitHook();

}
