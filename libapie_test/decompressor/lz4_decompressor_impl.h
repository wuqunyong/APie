#pragma once

#include <lz4.h>
#include <lz4hc.h>
#include <lz4frame.h>

#include "decompressor.h"

namespace APie {
namespace Decompressor {

/**
 * Implementation of decompressor's interface.
 */
class LZ4DecompressorImpl : public Decompressor {
public:
	LZ4DecompressorImpl();
	~LZ4DecompressorImpl();

  // Decompressor
	std::optional<std::string> decompress(const std::string& in) override;

private:
	std::optional<std::string> doUncompress(const std::string& in);
};

} // namespace Decompressor
} // namespace APie
