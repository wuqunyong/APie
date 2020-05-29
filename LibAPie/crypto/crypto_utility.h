#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace APie {
namespace Crypto {

class Utility {
public:
	static std::string hex(const std::string& src);
	static std::string hex(const uint8_t* src, size_t srcLen);

	static std::string sha1Hex(const std::string& src);
	static std::string md5(const std::string& src);
};

} // namespace Crypto
} // namespace APie
