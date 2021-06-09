#include "lz4_compressor_impl.h"

namespace APie {
namespace Compressor {

	LZ4CompressorImpl::LZ4CompressorImpl()
	{

	}

	LZ4CompressorImpl::~LZ4CompressorImpl()
	{

	}

	std::optional<std::string> LZ4CompressorImpl::compress(const std::string& data, int level)
	{
		return doCompress(data, level);
	}

	std::optional<std::string> LZ4CompressorImpl::doCompress(const std::string& data, int level)
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

} // namespace Compressor
} // namespace APie
