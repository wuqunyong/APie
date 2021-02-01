#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <optional>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include "../singleton/threadsafe_singleton.h"

namespace APie {
namespace Crypto {

class Utility {
public:
	static std::string hex(const std::string& src);
	static std::string hex(const uint8_t* src, size_t srcLen);

	static std::string sha1(const std::string& src);
	static std::string md5(const std::string& src);
};

class RSAUtility
{
public:
	~RSAUtility();

	bool init(const std::string &pubFile, const std::string &priFile, std::string &errInfo);
	bool encrypt(const std::string& plainMsg, std::string *encryptedMsg);
	bool decrypt(const std::string& encryptedMsg, std::string* decryptedMsg);
	std::optional<int> rsaSize();

private:
	FILE *m_pub_file = nullptr;
	RSA *m_pub_key = nullptr;

	FILE *m_pri_file = nullptr;
	RSA *m_pri_key = nullptr;
};

using RSAUtilitySingleton = ThreadSafeSingleton<RSAUtility>;

} // namespace Crypto
} // namespace APie
