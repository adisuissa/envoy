#include "source/common/config/decoded_resource_impl.h"
#include "source/extensions/config/validators/minimum_clusters/minimum_clusters_validator.h"

#include "test/mocks/server/instance.h"
#include "test/mocks/upstream/cluster_manager.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace Config {
namespace Validators {
namespace {

class MinimumClustersValidatorTest : public testing::Test {
public:
  MinimumClustersValidatorTest() {
    ON_CALL(server_, clusterManager()).WillByDefault(ReturnRef(cluster_manager_));
  }

  NiceMock<Upstream::MockClusterManager> cluster_manager_;
  NiceMock<Server::MockInstance> server_;
};

// Validates that an empty config accepts an update with no clusters.
TEST_F(MinimumClustersValidatorTest, NoMinimumNoClusters) {
  envoy::extensions::config::validators::minimum_clusters::v3::MinimumClustersValidator config;
  Upstream::MockClusterManager::ClusterInfoMaps cluster_info{{}, {}, {}};
  MinimumClustersValidator validator(config);

  const std::vector<Envoy::Config::DecodedResourcePtr> added_resources;
  const Protobuf::RepeatedPtrField<std::string> removed_resources;

  EXPECT_CALL(cluster_manager_, clusters()).WillOnce(Return(cluster_info));
  EXPECT_TRUE(validator.validate(server_, added_resources, removed_resources));
}

// Validates that a config with threshold 1 rejects an update with no clusters.
TEST_F(MinimumClustersValidatorTest, Minimum1NoClusters) {
  envoy::extensions::config::validators::minimum_clusters::v3::MinimumClustersValidator config;
  config.set_min_clusters_num(1);
  Upstream::MockClusterManager::ClusterInfoMaps cluster_info{{}, {}, {}};
  MinimumClustersValidator validator(config);

  const std::vector<Envoy::Config::DecodedResourcePtr> added_resources;
  const Protobuf::RepeatedPtrField<std::string> removed_resources;
  EXPECT_CALL(cluster_manager_, clusters()).WillOnce(Return(cluster_info));
  EXPECT_THROW_WITH_MESSAGE(validator.validate(server_, added_resources, removed_resources),
                            EnvoyException,
                            "CDS update attempts to reduce clusters below configured minimum.");
}

// Validates that a config with threshold 1 and a config that has a cluster
// is accepted.
TEST_F(MinimumClustersValidatorTest, Minimum1SingleCluster) {
  envoy::extensions::config::validators::minimum_clusters::v3::MinimumClustersValidator config;
  Upstream::MockClusterManager::ClusterInfoMaps cluster_info{{}, {}, {}};
  MinimumClustersValidator validator(config);

  auto cluster = std::make_unique<envoy::config::cluster::v3::Cluster>();
  cluster->set_name("cluster1");
  std::vector<Envoy::Config::DecodedResourcePtr> added_resources;
  added_resources.emplace_back(new Envoy::Config::DecodedResourceImpl(std::move(cluster), "name", {}, "ver"));
  const Protobuf::RepeatedPtrField<std::string> removed_resources;
  EXPECT_CALL(cluster_manager_, clusters()).WillOnce(Return(cluster_info));
  EXPECT_TRUE(validator.validate(server_, added_resources, removed_resources));
}

// Validates that an empty config accepts an update with a single cluster.
TEST_F(MinimumClustersValidatorTest, NoMinimumSingleCluster) {
  envoy::extensions::config::validators::minimum_clusters::v3::MinimumClustersValidator config;
  NiceMock<Upstream::MockClusterMockPrioritySet> foo_cluster;
  Upstream::MockClusterManager::ClusterInfoMaps cluster_info{
      {{"foo", foo_cluster}}, {}, {{"foo", foo_cluster}}};
  MinimumClustersValidator validator(config);

  const std::vector<Envoy::Config::DecodedResourcePtr> added_resources;
  const Protobuf::RepeatedPtrField<std::string> removed_resources;
  EXPECT_CALL(cluster_manager_, clusters()).WillOnce(Return(cluster_info));
  EXPECT_TRUE(validator.validate(server_, added_resources, removed_resources));
}

// Validates that a config with threshold 1 and a server that already has a
// cluster, accepts an update with no clusters.
TEST_F(MinimumClustersValidatorTest, Minimum1Clusters1Empty) {
  envoy::extensions::config::validators::minimum_clusters::v3::MinimumClustersValidator config;
  config.set_min_clusters_num(1);
  NiceMock<Upstream::MockClusterMockPrioritySet> foo_cluster;
  Upstream::MockClusterManager::ClusterInfoMaps cluster_info{
      {{"foo", foo_cluster}}, {}, {{"foo", foo_cluster}}};
  MinimumClustersValidator validator(config);

  const std::vector<Envoy::Config::DecodedResourcePtr> added_resources;
  const Protobuf::RepeatedPtrField<std::string> removed_resources;
  EXPECT_CALL(cluster_manager_, clusters()).WillOnce(Return(cluster_info));
  EXPECT_TRUE(validator.validate(server_, added_resources, removed_resources));
}

// Validates that a config with threshold 1 and a server that already has 2
// clusters, accepts an update that removes a cluster.
TEST_F(MinimumClustersValidatorTest, Minimum1Clusters2Remove1) {
  envoy::extensions::config::validators::minimum_clusters::v3::MinimumClustersValidator config;
  config.set_min_clusters_num(1);
  NiceMock<Upstream::MockClusterMockPrioritySet> foo_cluster, bar_cluster;
  Upstream::MockClusterManager::ClusterInfoMaps cluster_info{
      {{"foo", foo_cluster}, {"bar", bar_cluster}},
      {},
      {{"foo", foo_cluster}, {"bar", bar_cluster}}};
  MinimumClustersValidator validator(config);

  const std::vector<Envoy::Config::DecodedResourcePtr> added_resources;
  Protobuf::RepeatedPtrField<std::string> removed_resources;
  removed_resources.Add("foo");
  EXPECT_CALL(cluster_manager_, clusters()).WillOnce(Return(cluster_info));
  EXPECT_TRUE(validator.validate(server_, added_resources, removed_resources));
}

// Validates that a config with threshold 1 and a server that already has 1
// cluster, accepts an update that removes a non-existent cluster.
TEST_F(MinimumClustersValidatorTest, Minimum1Clusters1RemoveNonExistent) {
  envoy::extensions::config::validators::minimum_clusters::v3::MinimumClustersValidator config;
  config.set_min_clusters_num(1);
  NiceMock<Upstream::MockClusterMockPrioritySet> foo_cluster;
  Upstream::MockClusterManager::ClusterInfoMaps cluster_info{
      {{"foo", foo_cluster}}, {}, {{"foo", foo_cluster}}};
  MinimumClustersValidator validator(config);

  const std::vector<Envoy::Config::DecodedResourcePtr> added_resources;
  Protobuf::RepeatedPtrField<std::string> removed_resources;
  removed_resources.Add("bar");
  EXPECT_CALL(cluster_manager_, clusters()).WillOnce(Return(cluster_info));
  EXPECT_TRUE(validator.validate(server_, added_resources, removed_resources));
}

// Validates that a config with threshold 1 and a server that already has 1
// cluster, rejects an update that removes the cluster.
TEST_F(MinimumClustersValidatorTest, Minimum1Clusters1Remove1) {
  envoy::extensions::config::validators::minimum_clusters::v3::MinimumClustersValidator config;
  config.set_min_clusters_num(1);
  NiceMock<Upstream::MockClusterMockPrioritySet> foo_cluster;
  Upstream::MockClusterManager::ClusterInfoMaps cluster_info{
      {{"foo", foo_cluster}}, {}, {{"foo", foo_cluster}}};
  MinimumClustersValidator validator(config);

  const std::vector<Envoy::Config::DecodedResourcePtr> added_resources;
  Protobuf::RepeatedPtrField<std::string> removed_resources;
  removed_resources.Add("foo");
  // Only clusters that were added by API (CDS) can be removed.
  EXPECT_CALL(cluster_manager_, clusters()).WillOnce(Return(cluster_info));
  EXPECT_CALL(*foo_cluster.info_, addedViaApi()).WillOnce(Return(true));
  EXPECT_THROW_WITH_MESSAGE(validator.validate(server_, added_resources, removed_resources),
                            EnvoyException,
                            "CDS update attempts to reduce clusters below configured minimum.");
}

// Validates that a config with threshold 1 and a server that already has 1
// cluster that wasn't updated via API, accepts an update that removes the cluster.
TEST_F(MinimumClustersValidatorTest, Minimum1Clusters1Remove1NonApi) {
  envoy::extensions::config::validators::minimum_clusters::v3::MinimumClustersValidator config;
  config.set_min_clusters_num(1);
  NiceMock<Upstream::MockClusterMockPrioritySet> foo_cluster;
  Upstream::MockClusterManager::ClusterInfoMaps cluster_info{
      {{"foo", foo_cluster}}, {}, {{"foo", foo_cluster}}};
  MinimumClustersValidator validator(config);

  const std::vector<Envoy::Config::DecodedResourcePtr> added_resources;
  Protobuf::RepeatedPtrField<std::string> removed_resources;
  removed_resources.Add("foo");
  EXPECT_CALL(cluster_manager_, clusters()).WillOnce(Return(cluster_info));
  EXPECT_TRUE(validator.validate(server_, added_resources, removed_resources));
}

// Add a test with high threshold, and small update (only additions), and make
// sure it is rejected
} // namespace
} // namespace Validators
} // namespace Config
} // namespace Extensions
} // namespace Envoy
