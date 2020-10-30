#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

#include "apie.h"
#include "json/json.h"

#include "service_init.h"

#include <lz4.h>
#include <lz4hc.h>
#include <lz4frame.h>

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
		Json::Reader reader;
		reader.parse(rawJson, root);
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

int main(int argc, char **argv)
{
	//std::string data = "The other signatures ((2) and (3)) are never called by a delete[]-expression (the delete[] operator always calls the ordinary version of this function, and exactly once for each of its arguments). These other signatures are only called automatically by a new[]-expression when their object construction fails (e.g., if the constructor of an object throws while being constructed by a new[]-expression with nothrow, the matching operator delete[] function accepting a nothrow argument is called).";

	TestJsonCpp();

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

	if (argc != 2)
	{
		fatalExit("usage: exe <ConfFile>");
	}

	std::string configFile = argv[1];

	APie::Hook::HookRegistrySingleton::get().appendHook(APie::Hook::HookPoint::HP_Init, APie::initHook);
	APie::Hook::HookRegistrySingleton::get().appendHook(APie::Hook::HookPoint::HP_Start, APie::startHook);
	APie::Hook::HookRegistrySingleton::get().appendHook(APie::Hook::HookPoint::HP_Ready, APie::readyHook);
	APie::Hook::HookRegistrySingleton::get().appendHook(APie::Hook::HookPoint::HP_Exit, APie::exitHook);

	APie::CtxSingleton::get().init(configFile);
	APie::CtxSingleton::get().start();
	APie::CtxSingleton::get().waitForShutdown();

	return 0;
}
