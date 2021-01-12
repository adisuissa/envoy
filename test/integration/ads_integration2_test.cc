#include "envoy/config/bootstrap/v3/bootstrap.pb.h"
#include "envoy/config/cluster/v3/cluster.pb.h"
#include "envoy/config/endpoint/v3/endpoint.pb.h"
#include "envoy/config/listener/v3/listener.pb.h"
#include "envoy/config/route/v3/route.pb.h"
#include "envoy/grpc/status.h"

#include "common/config/protobuf_link_hacks.h"
#include "common/config/version_converter.h"
#include "common/protobuf/protobuf.h"
#include "common/protobuf/utility.h"
#include "common/version/version.h"

#include "test/common/grpc/grpc_client_integration.h"
#include "test/integration/ads_integration2.h"
#include "test/integration/http_integration.h"
#include "test/integration/utility.h"
#include "test/test_common/network_utility.h"
#include "test/test_common/resources.h"
#include "test/test_common/utility.h"

#include "gtest/gtest.h"

using testing::AssertionResult;

namespace Envoy {

INSTANTIATE_TEST_SUITE_P(IpVersionsClientTypeDelta, AdsIntegration2Test,
                         DELTA_SOTW_GRPC_CLIENT_INTEGRATION_PARAMS);

// Validate basic config delivery and upgrade.
TEST_P(AdsIntegration2Test, Basic) {
  initialize();
  testBasicFlow();
}

// TODO(adisuissa) - this should be in the regular ADS integration test
// Validate that Node message is well formed.
// This is for V3 only and should be merged with the NodeMessage test once V2 is
// removed.
TEST_P(AdsIntegration2Test, NodeMessageV3Only) {
  initialize();
  envoy::service::discovery::v3::DiscoveryRequest sotw_request;
  envoy::service::discovery::v3::DeltaDiscoveryRequest delta_request;
  const envoy::config::core::v3::Node* node = nullptr;
  if (sotw_or_delta_ == Grpc::SotwOrDelta::Sotw) {
    EXPECT_TRUE(xds_stream_->waitForGrpcMessage(*dispatcher_, sotw_request));
    EXPECT_TRUE(sotw_request.has_node());
    node = &sotw_request.node();
  } else {
    EXPECT_TRUE(xds_stream_->waitForGrpcMessage(*dispatcher_, delta_request));
    EXPECT_TRUE(delta_request.has_node());
    node = &delta_request.node();
  }
  EXPECT_THAT(node->user_agent_build_version(), ProtoEq(VersionInfo::buildVersion()));
  EXPECT_THAT(node->user_agent_max_api_version(), ProtoEq(VersionInfo::apiVersion()));
  EXPECT_THAT(node->user_agent_min_api_version(), ProtoEq(VersionInfo::oldestApiVersion()));
  EXPECT_GE(node->extensions().size(), 0);
  EXPECT_EQ(0, node->client_features().size());
  xds_stream_->finishGrpcStream(Grpc::Status::Ok);
}

TEST_P(AdsIntegration2Test, TestMinorVersionBasicFlow) {
  initialize();

  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Cluster, "", {}, {}, {}, true));
  envoy::service::discovery::v3::DiscoveryResponseStatus response_status;
  response_status.set_type(envoy::service::discovery::v3::DiscoveryResponseStatus::OK);
  sendDiscoveryResponse<envoy::config::cluster::v3::Cluster>(
      Config::TypeUrl::get().Cluster, {buildCluster("cluster_0")}, {buildCluster("cluster_0")}, {},
      "1", false, response_status);

  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().ClusterLoadAssignment, "",
                                      {"cluster_0"}, {"cluster_0"}, {}));
  sendDiscoveryResponse<envoy::config::endpoint::v3::ClusterLoadAssignment>(
      Config::TypeUrl::get().ClusterLoadAssignment, {buildClusterLoadAssignment("cluster_0")},
      {buildClusterLoadAssignment("cluster_0")}, {}, "1");

  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Cluster, "1", {}, {}, {}));
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Listener, "", {}, {}, {}));
  sendDiscoveryResponse<envoy::config::listener::v3::Listener>(
      Config::TypeUrl::get().Listener, {buildListener("listener_0", "route_config_0")},
      {buildListener("listener_0", "route_config_0")}, {}, "1");

  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().ClusterLoadAssignment, "1",
                                      {"cluster_0"}, {}, {}));
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().RouteConfiguration, "",
                                      {"route_config_0"}, {"route_config_0"}, {}));
  sendDiscoveryResponse<envoy::config::route::v3::RouteConfiguration>(
      Config::TypeUrl::get().RouteConfiguration, {buildRouteConfig("route_config_0", "cluster_0")},
      {buildRouteConfig("route_config_0", "cluster_0")}, {}, "1");

  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Listener, "1", {}, {}, {}));
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().RouteConfiguration, "1",
                                      {"route_config_0"}, {}, {}));

  test_server_->waitForCounterGe("listener_manager.listener_create_success", 1);
  makeSingleRequest();
}

TEST_P(AdsIntegration2Test, TestIncorrectMinorVersion) {
  initialize();

  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Cluster, "", {}, {}, {}, true));
  envoy::service::discovery::v3::DiscoveryResponseStatus response_status;
  // Reply that no matching resources were found.
  response_status.set_type(
      envoy::service::discovery::v3::DiscoveryResponseStatus::NO_MATCHING_RESOURCES);
  sendDiscoveryResponse<envoy::config::cluster::v3::Cluster>(Config::TypeUrl::get().Cluster, {}, {},
                                                             {}, "1", false, response_status);

  // test_server_->waitForCounterGe("cluster_manager.cds.update_rejected", 1);
}

TEST_P(AdsIntegration2Test, TestCorrectThenIncorrectMinorVersion) {
  initialize();

  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Cluster, "", {}, {}, {}, true));
  envoy::service::discovery::v3::DiscoveryResponseStatus response_status;
  response_status.set_type(envoy::service::discovery::v3::DiscoveryResponseStatus::OK);
  sendDiscoveryResponse<envoy::config::cluster::v3::Cluster>(
      Config::TypeUrl::get().Cluster, {buildCluster("cluster_0")}, {buildCluster("cluster_0")}, {},
      "1", false, response_status);

  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().ClusterLoadAssignment, "",
                                      {"cluster_0"}, {"cluster_0"}, {}));
  sendDiscoveryResponse<envoy::config::endpoint::v3::ClusterLoadAssignment>(
      Config::TypeUrl::get().ClusterLoadAssignment, {buildClusterLoadAssignment("cluster_0")},
      {buildClusterLoadAssignment("cluster_0")}, {}, "1");

  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Cluster, "1", {}, {}, {}));
  EXPECT_TRUE(compareDiscoveryRequest(Config::TypeUrl::get().Listener, "", {}, {}, {}));

  // Reply that no matching resources were found.
  response_status.set_type(
      envoy::service::discovery::v3::DiscoveryResponseStatus::NO_MATCHING_RESOURCES);
  sendDiscoveryResponse<envoy::config::listener::v3::Listener>(
      Config::TypeUrl::get().Listener, {buildListener("listener_0", "route_config_0")},
      {buildListener("listener_0", "route_config_0")}, {}, "1", false, response_status);

  // test_server_->waitForCounterGe("listener_manager.lds.update_rejected", 1);
}

// Send without minor version
// Send with minor version range that is too old or too new
// Send with an overlapping version range

// Assumptions:
// - Client will not send a range across different major versions
// - Client requests a range of minor versions, and receives resources of any of
// any of these versions (can be mixed), and doesn't need to verify that each
// resource version is in range.
// - If the client doesn't send range of supported versions (major version is a
// must), the server can send any minor version that it has.

} // namespace Envoy
