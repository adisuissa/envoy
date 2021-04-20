#include "common/version/api_version.h"

namespace Envoy {

envoy::type::v3::SemanticVersion
ApiVersionInfo::convertToSemanticVersion(const ApiVersion& api_version) {
  envoy::type::v3::SemanticVersion semantic_version;
  semantic_version.set_major_number(api_version.major);
  semantic_version.set_minor_number(api_version.minor);
  semantic_version.set_patch(api_version.patch);
  return semantic_version;
}

} // namespace Envoy
