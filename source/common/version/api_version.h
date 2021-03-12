#pragma once

#include <string>

#include "common/version/api_version_struct.h"

// Defines the ApiVersion version (Envoy::api_version).
#include "common/version/api_version_number.h"

namespace Envoy {

/**
 * Wraps compiled in api versioning.
 */
class ApiVersionInfo {
public:
  // Returns the most recent API version that is supported by the client.
  static const ApiVersion& apiVersion();

  // Returns the oldest API version that is supported by the client.
  static const ApiVersion& oldestApiVersion();
};

} // namespace Envoy
