#pragma once

#include <string>
#include <optional>

#include "../common/pure.h"

namespace APie {
namespace Decompressor {

/**
 * Allows decompressing data.
 */
class Decompressor {
public:
  virtual ~Decompressor() {}

  virtual std::optional<std::string> decompress(const std::string& in) PURE;
};

} // namespace Decompressor
} // namespace Envoy
