#include "common/version/version.h"

#include <map>
#include <regex>
#include <string>

#include "common/common/fmt.h"
#include "common/common/macros.h"
#include "common/protobuf/utility.h"
#include "common/version/api_version_number.h"

#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"

extern const char build_scm_revision[];
extern const char build_scm_status[];

namespace Envoy {
const std::string& VersionInfo::revision() {
  CONSTRUCT_ON_FIRST_USE(std::string, build_scm_revision);
}

const std::string& VersionInfo::revisionStatus() {
  CONSTRUCT_ON_FIRST_USE(std::string, build_scm_status);
}

const std::string& VersionInfo::version() {
  CONSTRUCT_ON_FIRST_USE(std::string,
                         fmt::format("{}/{}/{}/{}/{}", revision(), BUILD_VERSION_NUMBER,
                                     revisionStatus(), buildType(), sslVersion()));
}

std::string
VersionInfo::apiVersionToString(const envoy::config::core::v3::ApiVersionNumber& version) {
  return fmt::format("{}.{}.{}", version.version().major_number(), version.version().minor_number(),
                     version.version().patch());
}

const envoy::config::core::v3::BuildVersion& VersionInfo::buildVersion() {
  static const auto* result =
      new envoy::config::core::v3::BuildVersion(makeBuildVersion(BUILD_VERSION_NUMBER));
  return *result;
}

const envoy::config::core::v3::ApiVersionNumber& VersionInfo::apiVersion() {
  static const auto* result =
      new envoy::config::core::v3::ApiVersionNumber(makeApiVersion(API_VERSION_NUMBER));
  return *result;
}

const envoy::config::core::v3::ApiVersionNumber& VersionInfo::oldestApiVersion() {
  static const auto* result =
      new envoy::config::core::v3::ApiVersionNumber(computeOldestApiVersion(apiVersion()));
  return *result;
}

const std::string& VersionInfo::buildType() {
#ifdef NDEBUG
  static const std::string release_type = "RELEASE";
#else
  static const std::string release_type = "DEBUG";
#endif
  return release_type;
}

const std::string& VersionInfo::sslVersion() {
#ifdef ENVOY_SSL_VERSION
  static const std::string ssl_version = ENVOY_SSL_VERSION;
#else
  static const std::string ssl_version = "no-ssl";
#endif
  return ssl_version;
}

envoy::config::core::v3::BuildVersion VersionInfo::makeBuildVersion(const char* version) {
  envoy::config::core::v3::BuildVersion result;
  // Split BUILD_VERSION_NUMBER into version and an optional build label after the '-'
  std::regex ver_regex("([\\d]+)\\.([\\d]+)\\.([\\d]+)(-(.*))?");
  // Match indexes, given the regex above
  constexpr std::cmatch::size_type major = 1;
  constexpr std::cmatch::size_type minor = 2;
  constexpr std::cmatch::size_type patch = 3;
  constexpr std::cmatch::size_type label = 5;
  std::cmatch match;
  if (std::regex_match(version, match, ver_regex)) {
    int value = 0;
    if (absl::SimpleAtoi(match.str(major), &value)) {
      result.mutable_version()->set_major_number(value);
    }
    if (absl::SimpleAtoi(match.str(minor), &value)) {
      result.mutable_version()->set_minor_number(value);
    }
    if (absl::SimpleAtoi(match.str(patch), &value)) {
      result.mutable_version()->set_patch(value);
    }
  }
  std::map<std::string, std::string> fields;
  if (!match.str(label).empty()) {
    fields[BuildVersionMetadataKeys::get().BuildLabel] = match.str(label);
  }
  fields[BuildVersionMetadataKeys::get().BuildType] = buildType();
  fields[BuildVersionMetadataKeys::get().SslVersion] = sslVersion();
  fields[BuildVersionMetadataKeys::get().RevisionSHA] = revision();
  fields[BuildVersionMetadataKeys::get().RevisionStatus] = revisionStatus();
  *result.mutable_metadata() = MessageUtil::keyValueStruct(fields);
  return result;
}

envoy::config::core::v3::ApiVersionNumber VersionInfo::makeApiVersion(const char* version) {
  envoy::config::core::v3::ApiVersionNumber result;
  // Split API_VERSION_NUMBER into version
  std::regex ver_regex("([\\d]+)\\.([\\d]+)\\.([\\d]+)");
  // Match indexes, given the regex above
  constexpr std::cmatch::size_type major = 1;
  constexpr std::cmatch::size_type minor = 2;
  constexpr std::cmatch::size_type patch = 3;
  std::cmatch match;
  if (std::regex_match(version, match, ver_regex)) {
    uint32_t value = 0;
    if (absl::SimpleAtoi(match.str(major), &value)) {
      result.mutable_version()->set_major_number(value);
    }
    if (absl::SimpleAtoi(match.str(minor), &value)) {
      result.mutable_version()->set_minor_number(value);
    }
    if (absl::SimpleAtoi(match.str(patch), &value)) {
      result.mutable_version()->set_patch(value);
    }
  }
  return result;
}

envoy::config::core::v3::ApiVersionNumber VersionInfo::computeOldestApiVersion(
    const envoy::config::core::v3::ApiVersionNumber& latest_version) {
  envoy::config::core::v3::ApiVersionNumber result;
  // The oldest API version that is supported by the client is up to 2 minor
  // versions before the latest version. Note that the major number is always
  // the same as the latest version, and the patch number is always 0. This
  // implies that the minor number is at least 0, and the oldest api version
  // cannot be set to a previous major number.
  result.mutable_version()->set_major_number(latest_version.version().major_number());
  result.mutable_version()->set_minor_number(std::max(
      static_cast<int64_t>(latest_version.version().minor_number()) - 2, static_cast<int64_t>(0)));
  result.mutable_version()->set_patch(0);
  return result;
}
} // namespace Envoy
