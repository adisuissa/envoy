#include "common/version/api_version.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Envoy {

// Verify assumptions about oldest version vs latest version.
TEST(ApiVersionTest, OldestLatestVersionsAssumptions) {
  constexpr auto latest_version = ApiVersionInfo::apiVersion();
  constexpr auto oldest_version = ApiVersionInfo::oldestApiVersion();
  // Same major number, minor number difference is at most 1, and the oldest patch is 0.
  EXPECT_EQ(latest_version.major, oldest_version.major);
  EXPECT_TRUE(latest_version.minor >= oldest_version.minor);
  if (latest_version.minor > 0) {
    EXPECT_EQ(latest_version.minor - 1, oldest_version.minor);
  }
  EXPECT_EQ(0, oldest_version.patch);
}

// Test the ApiVersion to SemanticVersion conversion.
TEST(ApiVersionTest, ApiVersionToSemanticVersionConversion) {
  std::vector<ApiVersion> api_versions{ApiVersionInfo::apiVersion(),
                                       ApiVersionInfo::oldestApiVersion(), ApiVersion{0, 0, 0},
                                       ApiVersion{1, 2, 3}};
  for (const auto& api_version : api_versions) {
    const auto semantic_version = ApiVersionInfo::convertToSemanticVersion(api_version);
    EXPECT_EQ(api_version.major, semantic_version.major_number());
    EXPECT_EQ(api_version.minor, semantic_version.minor_number());
    EXPECT_EQ(api_version.patch, semantic_version.patch());
  }
}

} // namespace Envoy
