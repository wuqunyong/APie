#include "crypto_utility.h"

#include <stdint.h>
#include <algorithm>
#include <functional>

#include <openssl/sha.h>
#include <openssl/md5.h>

namespace APie {
namespace Crypto {

	std::string Utility::hex(const uint8_t* src, size_t srcLen)
	{
		size_t iLen = srcLen * 2 + 1;
		char *mdString = new char[iLen];
		memset(mdString, 0, iLen);

		for (size_t i = 0; i < srcLen; i++)
		{
			sprintf(&mdString[i * 2], "%02x", (unsigned int)src[i]);
		}

		std::string hexStr(mdString);
		delete[]mdString;

		return hexStr;
	}

	std::string Utility::hex(const std::string& src)
	{
		return hex((const uint8_t*)src.data(), src.size());
	}

	std::string Utility::sha1Hex(const std::string& src)
	{
		uint8_t output[20] = { '\0' };
		//sha1((const uint8_t*)src.data(), src.size(), output);
		size_t iLen = src.length();
		SHA1((unsigned char*)src.c_str(), iLen, (unsigned char*)&output);

		return hex(output, 20);
	}

	std::string Utility::md5(const std::string& src)
	{
		unsigned char digest[MD5_DIGEST_LENGTH];
		MD5((unsigned char*)src.c_str(), src.length(), (unsigned char*)&digest);

		return hex(digest, 16);
	}

} // namespace Crypto
} // namespace APie
