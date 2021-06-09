#include "lz4_decompressor_impl.h"

namespace APie {
namespace Decompressor {

	LZ4DecompressorImpl::LZ4DecompressorImpl()
	{

	}

	LZ4DecompressorImpl::~LZ4DecompressorImpl()
	{

	}

	// Decompressor
	std::optional<std::string> LZ4DecompressorImpl::decompress(const std::string& in)
	{
		return doUncompress(in);
	}

	std::optional<std::string> LZ4DecompressorImpl::doUncompress(const std::string& in)
	{
		constexpr size_t blockSize = uint64_t{ 2 } << 20;

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
			if (LZ4F_isError(code))
			{
				std::string sError = LZ4F_getErrorName(code);
				return std::nullopt;
			}
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

} // namespace Decompressor
} // namespace APie
