#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>
#include <iostream>
#include <type_traits>
#include <variant>

#include "apie.h"
#include "json/json.h"
//#include "tinyxml2.h"

#include "service_init.h"

#include <lz4.h>
#include <lz4hc.h>
#include <lz4frame.h>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rc4.h>

#include "../libapie/common/file.h"
#include "../pb_msg/business/login_msg.pb.h"

//using namespace tinyxml2;
using namespace std;

constexpr size_t kMaxVarintLength64 = 10;

std::optional<std::string> doCompress(const std::string& data, int level)
{
	const auto uncompressedLength = data.size();

	LZ4F_preferences_t prefs{};
	prefs.compressionLevel = level;
	prefs.frameInfo.contentSize = uncompressedLength;

	// Compress
	auto iComressedLen = LZ4F_compressFrameBound(uncompressedLength, &prefs); //LZ4_compressBound(uncompressedLength);
	auto buf = new char[iComressedLen];

	auto iRC = LZ4F_compressFrame(
		buf,
		iComressedLen,
		data.data(),
		data.length(),
		&prefs);

	if (LZ4F_isError(iRC))
	{
		delete[] buf;
		std::string sError = LZ4F_getErrorName(iRC);
		return std::nullopt;
	}

	std::string compressedBuf(buf, iRC);
	delete[] buf;

	return compressedBuf;
}

std::optional<std::string> doUncompress(const std::string& in)
{
	//constexpr size_t blockSize = uint64_t{ 4 } << 20;
	constexpr size_t blockSize = 128;

	LZ4F_decompressionContext_t dctx{ nullptr };
	auto iRC = LZ4F_createDecompressionContext(&dctx, 100);
	if (LZ4F_isError(iRC))
	{
		std::string sError = LZ4F_getErrorName(iRC);
		return std::nullopt;
	}

	// Select decompression options
	LZ4F_decompressOptions_t options;
	options.stableDst = 1;

	std::string result;

	// Decompress until the frame is over
	size_t code = 0;
	size_t iAdvance = 0;

	// Allocate enough space to decompress at least a block
	size_t outSize = blockSize;
	auto out = new char[outSize];

	do {
		// Decompress
		size_t inSize = in.size() - iAdvance;
		code = LZ4F_decompress(dctx, out, &outSize, in.data() + iAdvance, &inSize, &options);
		iAdvance += inSize;

		result.append(out, outSize);
	} while (code != 0);

	delete[] out;

	if (dctx) 
	{
		LZ4F_freeDecompressionContext(dctx);
	}

	return result;
}

int TestJsonCpp()
{
	const std::string rawJson = R"({"Age": 20, "Name": "colin"})";
	const auto rawJsonLength = static_cast<int>(rawJson.length());
	constexpr bool shouldUseOldWay = false;
	std::string err;
	Json::Value root;

	if (shouldUseOldWay) {
		//Json::Reader reader;
		//reader.parse(rawJson, root);
	}
	else {
		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		if (!reader->parse(rawJson.c_str(), rawJson.c_str() + rawJsonLength, &root,
			&err)) {
			std::cout << "error" << std::endl;
			return EXIT_FAILURE;
		}
	}
	const std::string name = root["Name"].asString();
	const int age = root["Age"].asInt();

	return EXIT_SUCCESS;
}

//int TestXML()
//{
//
//	static const char* xml =
//		"<?xml version=\"1.0\"?>"
//		"<!DOCTYPE PLAY SYSTEM \"play.dtd\">"
//		"<PLAY>"
//		"<TITLE>A Midsummer Night's Dream</TITLE>"
//		"</PLAY>";
//
//	tinyxml2::XMLDocument doc;
//	doc.Parse(xml);
//
//	XMLElement* titleElement = doc.FirstChildElement("PLAY")->FirstChildElement("TITLE");
//	const char* title = titleElement->GetText();
//	printf("Name of play (1): %s\n", title);
//
//	XMLText* textNode = titleElement->FirstChild()->ToText();
//	title = textNode->Value();
//	printf("Name of play (2): %s\n", title);
//
//	return doc.ErrorID();
//}

bool TestEncrypt(std::string plainText, std::string *ptrCipherText)
{
	if (ptrCipherText == nullptr)
	{
		return false;
	}

	std::string key_path("E:\\APie\\conf\\key.pub");

	/// 打开 公钥 文件
	FILE *_file;
	_file = fopen(key_path.c_str(), "rb");
	if (_file == nullptr) {
		perror("open public file failed !!! \n");
		return false;
	}

	/// 从文件中获取 公钥
	RSA  *p_rsa = nullptr;
	p_rsa = PEM_read_RSA_PUBKEY(_file, NULL, NULL, NULL);
	if (p_rsa == nullptr) {
		perror("PEM_read_RSA_PUBKEY failed !!! \n");
		return false;
	}

	auto iRSASize = RSA_size(p_rsa);
	ptrCipherText->resize(iRSASize);

	/// 对内容进行加密
	int encrypted_size = RSA_public_encrypt(plainText.size(),
		(uint8_t *)plainText.data(),
		reinterpret_cast<uint8_t*>(&(*ptrCipherText)[0]),
		p_rsa,
		RSA_PKCS1_OAEP_PADDING);

	if (encrypted_size != static_cast<int>(iRSASize)) 
	{
		std::stringstream ss;
		ss << "RSA public encrypt failure: " << ERR_error_string(ERR_get_error(), NULL);
		return false;
	}

	if (p_rsa)    RSA_free(p_rsa);
	if (_file)     fclose(_file);

	return true;
}

bool TestDecrypt(const std::string& encrypted_message, std::string* decrypted_message) {
	std::string key_path("E:\\APie\\conf\\key.pem");

	// 打开 私钥 文件
	FILE *_file;
	_file = fopen(key_path.c_str(), "rb");
	if (_file == nullptr) {
		perror("open private file failed !!! \n");
		return false;
	}

	/// 从文件中获取 私钥
	RSA  *p_rsa = nullptr;
	p_rsa = PEM_read_RSAPrivateKey(_file, NULL, NULL, NULL);
	if (p_rsa == nullptr) {
		perror("PEM_read_RSAPrivateKey failed !!! \n");
		return false;
	}



	size_t rsa_size = RSA_size(p_rsa);
	if (encrypted_message.size() != rsa_size) {
		std::stringstream ss;
		ss << "Encrypted RSA message has the wrong size (expected "
			<< rsa_size << ", actual " << encrypted_message.size() << ").";
		return false;
	}

	decrypted_message->resize(rsa_size);
	int decrypted_size = RSA_private_decrypt(
		rsa_size, reinterpret_cast<const uint8_t*>(encrypted_message.data()),
		reinterpret_cast<uint8_t*>(&(*decrypted_message)[0]), p_rsa,
		RSA_PKCS1_OAEP_PADDING);

	if (decrypted_size == -1) {
		std::stringstream ss;
		ss << "RSA private decrypt failure: " << ERR_error_string(
			ERR_get_error(), NULL);
		return false;
	}
	decrypted_message->resize(decrypted_size);
	return true;
}

uint64_t MakeKey(uint32_t high, uint32_t low)
{
	uint32_t factor = 100000000;

	uint64_t result = high;
	result = result * factor + low;
	return result;
}

enum MyEnum : uint32_t
{
	ME_None = 0,
	ME_Hehlo = 1,
};

string decode_rc4(const std::string& sharedKey, const std::string& data) {
	if (data.empty())
	{
		return data;
	}

	RC4_KEY key;

	int len = data.size();

	string decode_data;
	decode_data.resize(len);

	//unsigned char *obuf = (unsigned char*)malloc(len + 1);
	//memset(obuf, 0, len + 1);

	RC4_set_key(&key, sharedKey.size(), (const unsigned char*)&sharedKey[0]);
	RC4(&key, len, (const unsigned char*)data.c_str(), (unsigned char *)&decode_data[0]);

	//string decode_data((char*)obuf, len);
	//free(obuf);

	return decode_data;
}

string encode_rc4(const std::string& sharedKey, const string& data) {
	
	if (data.empty())
	{
		return data;
	}

	RC4_KEY key;
	int len = data.size();

	string encode_data;
	encode_data.resize(len);

	//unsigned char *obuf = (unsigned char*)malloc(len + 1);
	//memset(obuf, 0, len + 1);

	RC4_set_key(&key, sharedKey.size(), (const unsigned char*)&sharedKey[0]);
	RC4(&key, len, (const unsigned char*)data.c_str(), (unsigned char *)&encode_data[0]);

	//string encode_data((char*)obuf, len);
	//free(obuf);

	return encode_data;
}

class TestType
{
public:
	int PrintA(int a, int b)
	{
		return 0;
	}

	static void handleRespAddRoute(uint64_t iSerialNum, const ::login_msg::MSG_RESPONSE_ACCOUNT_LOGIN_L& response)
	{

	}

	int operator()(int a, int b, int c)
	{
		return 0;
	}
};

template<std::size_t... I>
std::size_t a2t_impl()
{
	std::index_sequence<I...> a;
	return a.size();
	//return (0 + ...+ value);
}


template<typename... Args, std::size_t... I>
std::string gen_key_impl(std::tuple<Args...> &&params, std::index_sequence<I...>)
{
	std::stringstream ss;
	((ss << (I == 0 ? "" : ":") << std::get<I>(params)), ...);

	return ss.str();
}

template<typename... Args>
std::string gen_key(Args... args)
{
	return gen_key_impl(std::forward<std::tuple<Args...>>(std::make_tuple(args...)), std::index_sequence_for<Args...>{});
}


//const ::login_msg::MSG_RESPONSE_ACCOUNT_LOGIN_L& a
int test_func_1(int serialNum, const ::login_msg::MSG_RESPONSE_ACCOUNT_LOGIN_L& a)
{
	return 0;
}

class TestMakeObj
{
public:
	static std::shared_ptr<TestMakeObj> makeObj()
	{
		return std::make_shared<TestMakeObj>();
	}
	

public:
	TestMakeObj() {
		i = 100;
	};
	int i = 0;

};

template<class Tuple, std::size_t N>
struct TuplePrinter {
	static void print(const Tuple& t)
	{
		TuplePrinter<Tuple, N - 1>::print(t);
		std::cout << ", " << std::get<N - 1>(t);
	}
};

template<class Tuple>
struct TuplePrinter<Tuple, 1> {
	static void print(const Tuple& t)
	{
		std::cout << std::get<0>(t);
	}
};

int main(int argc, char **argv)
{
	//std::tuple<int, float, std::string, int> tpl{ 4, 6.6, "hello", 7 };
	//TuplePrinter<decltype(tpl), std::tuple_size<decltype(tpl)>::value>::print(tpl);

	std::is_constructible<int>::value;

	auto pData = std::make_tuple(1, 2, 3, 4, 5);
	for (uint32_t i = 0; i < 5; i++)
	{
		std::cout << std::get<0>(pData);
	}

	auto ptrObj = TestMakeObj::makeObj();

	bool bResult111 = std::is_function<decltype(TestType::handleRespAddRoute)>::value;
	static_assert(std::is_function<decltype(TestType::handleRespAddRoute)>::value);

	auto iii = a2t_impl<1, 2, 3, 4, 5, 6>();
	std::string hello("hello");
	auto key_str = gen_key("player", 1, "register", hello);

	auto iR = MakeKey(9000001, 123);
	auto sSS = SSImpl("hello", 1, iR, "world");

	auto iValue = APie::toUnderlyingType(MyEnum::ME_Hehlo);

	std::string plainMsg("hello");
	std::string encryptedMsg;
	std::string decryptedMsg;

	TestJsonCpp();
	//TestXML();

	//TestEncrypt(plainMsg, &encryptedMsg);
	//TestDecrypt(encryptedMsg, &decryptedMsg);

	//std::string errInfo;
	//bool bResult = APie::Crypto::RSAUtilitySingleton::get().init("E:\\APie\\conf\\key.pub", "E:\\APie\\conf\\key.pem", errInfo);
	//if (!bResult)
	//{
	//	return 1;
	//}

	{

		std::string content;
		bool bResult = APie::Common::GetContent("E:\\APie\\conf\\key.pub", &content);

		const std::vector<uint8_t> keyDer(content.begin(), content.end());

		BIO* ptrBIO = BIO_new_mem_buf(&content[0], content.size());
		std::unique_ptr<BIO, decltype(BIO_free)*> bio(ptrBIO, BIO_free);

		RSA* ptrRSA = PEM_read_bio_RSA_PUBKEY(bio.get(), NULL, NULL, NULL);
		std::unique_ptr<RSA, decltype(RSA_free)*> rsa(ptrRSA, RSA_free);

		APie::Crypto::RSAUtilitySingleton::get().encryptByPub(rsa.get(), plainMsg, &encryptedMsg);
		APie::Crypto::RSAUtilitySingleton::get().decrypt(encryptedMsg, &decryptedMsg);

		ERR_clear_error();
	}


	APie::Crypto::RSAUtilitySingleton::get().encrypt(plainMsg, &encryptedMsg);
	APie::Crypto::RSAUtilitySingleton::get().decrypt(encryptedMsg, &decryptedMsg);


	std::string symCipher = encode_rc4("", "sssssssfsfsfsfs");
	std::string symPlain = decode_rc4("", symCipher);

	std::string data;
	auto optResult = doCompress(data, 0);
	if (optResult.has_value())
	{
		auto optOrigin = doUncompress(optResult.value());
		if (!optOrigin.has_value())
		{
			return 1;
		}

		bool bResult = false;
		if (data == optOrigin.value())
		{
			bResult = true;
		}
	}

	uint64_t iTotal = 0;
	std::string sTotal;

	std::vector<std::string> tables;
	tables.push_back("hello world");
	tables.push_back("hi");

	::mysql_proxy_msg::MysqlDescribeRequest args;
	for (const auto& table : tables)
	{
		auto ptrAdd = args.add_names();
		*ptrAdd = table;
	}


	auto iCurMs = APie::CtxSingleton::get().getCurMilliseconds();
	for (uint32_t i = 0; i < 10000; i++)
	{
		iTotal += APie::CtxSingleton::get().getCurMilliseconds();
		sTotal += args.SerializeAsString();
	}
	auto iEndMs = APie::CtxSingleton::get().getCurMilliseconds();

	auto iDeltaMs = iEndMs - iCurMs;


	if (argc != 2)
	{
		PANIC_ABORT("usage: exe <ConfFile>, Expected: %d, got: %d", 2, argc);
	}

	std::string configFile = argv[1];

	APie::Hook::HookRegistrySingleton::get().registerHook(APie::Hook::HookPoint::HP_Init, APie::initHook);
	APie::Hook::HookRegistrySingleton::get().registerHook(APie::Hook::HookPoint::HP_Start, APie::startHook);
	APie::Hook::HookRegistrySingleton::get().registerHook(APie::Hook::HookPoint::HP_Ready, APie::readyHook);
	APie::Hook::HookRegistrySingleton::get().registerHook(APie::Hook::HookPoint::HP_Exit, APie::exitHook);

	APie::CtxSingleton::get().init(configFile);
	APie::CtxSingleton::get().start();
	APie::CtxSingleton::get().waitForShutdown();

	return 0;
}
