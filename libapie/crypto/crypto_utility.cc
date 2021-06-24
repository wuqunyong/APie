#include "crypto_utility.h"

#include <algorithm>
#include <functional>
#include <cstring>
#include <sstream>

#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/rsa.h>


 #ifdef WIN32
 #include <openssl/applink.c>
 #endif


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

	std::string Utility::sha1(const std::string& src)
	{
		uint8_t output[SHA_DIGEST_LENGTH] = {'\0'};
		size_t iLen = src.length();
		SHA1((unsigned char*)src.c_str(), iLen, (unsigned char*)&output);

		return hex(output, SHA_DIGEST_LENGTH);
	}

	std::string Utility::md5(const std::string& src)
	{
		unsigned char digest[MD5_DIGEST_LENGTH] = {'\0'};
		MD5((unsigned char*)src.c_str(), src.length(), (unsigned char*)&digest);

		return hex(digest, MD5_DIGEST_LENGTH);
	}

	std::string Utility::decode_rc4(const std::string& sharedKey, const std::string& data) 
	{
		if (data.empty())
		{
			return data;
		}

		RC4_KEY key;

		int len = data.size();

		std::string decode_data;
		decode_data.resize(len);

		//unsigned char *obuf = (unsigned char*)malloc(len + 1);
		//memset(obuf, 0, len + 1);

		RC4_set_key(&key, sharedKey.size(), (const unsigned char*)&sharedKey[0]);
		RC4(&key, len, (const unsigned char*)data.c_str(), (unsigned char *)&decode_data[0]);

		//string decode_data((char*)obuf, len);
		//free(obuf);

		return decode_data;
	}

	std::string Utility::encode_rc4(const std::string& sharedKey, const std::string& data) {

		if (data.empty())
		{
			return data;
		}

		RC4_KEY key;
		int len = data.size();

		std::string encode_data;
		encode_data.resize(len);

		//unsigned char *obuf = (unsigned char*)malloc(len + 1);
		//memset(obuf, 0, len + 1);

		RC4_set_key(&key, sharedKey.size(), (const unsigned char*)&sharedKey[0]);
		RC4(&key, len, (const unsigned char*)data.c_str(), (unsigned char *)&encode_data[0]);

		//string encode_data((char*)obuf, len);
		//free(obuf);

		return encode_data;
	}

	RSAUtility::~RSAUtility()
	{
		if (m_pub_key)
		{
			RSA_free(m_pub_key);
			m_pub_key = nullptr;
		}

		if (m_pub_file)
		{
			fclose(m_pub_file);
			m_pub_file = nullptr;
		}

		if (m_pri_key)
		{
			RSA_free(m_pri_key);
			m_pri_key = nullptr;
		}

		if (m_pri_file)
		{
			fclose(m_pri_file);
			m_pri_file = nullptr;
		}
	}

	bool RSAUtility::init(const std::string &pubFile, const std::string &priFile, std::string &errInfo)
	{
		{
			/// 打开 公钥 文件
			m_pub_file = fopen(pubFile.c_str(), "rb");
			
			if (m_pub_file == nullptr)
			{
				std::stringstream ss;
				ss << "open public file failed !!!";

				errInfo = ss.str();
				return false;
			}

			/// 从文件中获取 公钥
			m_pub_key = PEM_read_RSA_PUBKEY(m_pub_file, NULL, NULL, NULL);
			if (m_pub_key == nullptr) 
			{
				std::stringstream ss;
				ss << "PEM_read_RSA_PUBKEY failed !!!";

				errInfo = ss.str();
				return false;
			}
		}

		{
			// 打开 私钥 文件
			m_pri_file = fopen(priFile.c_str(), "rb");
			if (m_pri_file == nullptr)
			{
				std::stringstream ss;
				ss << "open private file failed !!! \n";

				errInfo = ss.str();
				return false;
			}

			/// 从文件中获取 私钥
			m_pri_key = PEM_read_RSAPrivateKey(m_pri_file, NULL, NULL, NULL);
			if (m_pri_key == nullptr) 
			{
				std::stringstream ss;
				ss << "PEM_read_RSAPrivateKey failed !!!";

				errInfo = ss.str();
				return false;
			}
		}

		return true;
	}

	bool RSAUtility::encrypt(const std::string& plainMsg, std::string *encryptedMsg)
	{
		if (m_pub_key == nullptr)
		{
			return false;
		}

		auto iRSASize = RSA_size(m_pub_key);
		encryptedMsg->resize(iRSASize);

		/// 对内容进行加密
		int encrypted_size = RSA_public_encrypt(plainMsg.size(),
			reinterpret_cast<const uint8_t*>(plainMsg.data()),
			reinterpret_cast<uint8_t*>(&(*encryptedMsg)[0]),
			m_pub_key,
			RSA_PKCS1_OAEP_PADDING);

		if (encrypted_size != static_cast<int>(iRSASize))
		{
			std::stringstream ss;
			ss << "RSA public encrypt failure: " << ERR_error_string(ERR_get_error(), NULL);
			return false;
		}

		return true;
	}

	bool RSAUtility::encryptByPub(RSA* ptrPubKey, const std::string& plainMsg, std::string *encryptedMsg)
	{
		if (ptrPubKey == nullptr)
		{
			return false;
		}

		auto iRSASize = RSA_size(ptrPubKey);
		encryptedMsg->resize(iRSASize);

		/// 对内容进行加密
		int encrypted_size = RSA_public_encrypt(plainMsg.size(),
			reinterpret_cast<const uint8_t*>(plainMsg.data()),
			reinterpret_cast<uint8_t*>(&(*encryptedMsg)[0]),
			ptrPubKey,
			RSA_PKCS1_OAEP_PADDING);

		if (encrypted_size != static_cast<int>(iRSASize))
		{
			std::stringstream ss;
			ss << "RSA public encrypt failure: " << ERR_error_string(ERR_get_error(), NULL);
			return false;
		}

		return true;
	}

	bool RSAUtility::decrypt(const std::string& encryptedMsg, std::string* decryptedMsg)
	{
		if (m_pri_key == nullptr)
		{
			return false;
		}

		auto rsa_size = RSA_size(m_pri_key);
		if (encryptedMsg.size() != rsa_size)
		{
			std::stringstream ss;
			ss << "Encrypted RSA message has the wrong size (expected "
				<< rsa_size << ", actual " << encryptedMsg.size() << ").";
			return false;
		}

		decryptedMsg->resize(rsa_size);
		int decrypted_size = RSA_private_decrypt(
			rsa_size, reinterpret_cast<const uint8_t*>(encryptedMsg.data()),
			reinterpret_cast<uint8_t*>(&(*decryptedMsg)[0]), m_pri_key,
			RSA_PKCS1_OAEP_PADDING);

		if (decrypted_size == -1) 
		{
			std::stringstream ss;
			ss << "RSA private decrypt failure: " << ERR_error_string(
				ERR_get_error(), NULL);
			return false;
		}
		decryptedMsg->resize(decrypted_size);

		return true;
	}

	std::optional<int> RSAUtility::rsaSize()
	{
		if (m_pri_key == nullptr)
		{
			return std::nullopt;
		}

		auto iSize = RSA_size(m_pri_key);

		return iSize;
	}

} // namespace Crypto
} // namespace APie
