#pragma once

#include <optional>
#include <string>

#include "../common/pure.h"

namespace APie {
namespace Compressor {


/**
 * Allows compressing data.
 */
class Compressor {
public:
  virtual ~Compressor() {}


  virtual std::optional<std::string> compress(const std::string& data, int level) PURE;
};

} // namespace Compressor
} // namespace Envoy
