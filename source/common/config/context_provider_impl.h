#pragma once

#include "envoy/config/context_provider.h"

#include "common/common/callback_impl.h"
#include "common/common/thread.h"
#include "common/config/xds_context_params.h"

namespace Envoy {
namespace Config {

class ContextProviderImpl : public ContextProvider {
public:
  ContextProviderImpl(const envoy::config::core::v3::Node& node,
                      const Protobuf::RepeatedPtrField<std::string>& node_context_params);

  // Config::ContextProvider
  const xds::core::v3::ContextParams& nodeContext() const override { return node_context_; }

  const xds::core::v3::ContextParams&
  dynamicContext(absl::string_view resource_type_url) const override;
  void setDynamicContextParam(absl::string_view resource_type_url, absl::string_view key,
                              absl::string_view value) override;
  void unsetDynamicContextParam(absl::string_view resource_type_url,
                                absl::string_view key) override;
  ABSL_MUST_USE_RESULT Common::CallbackHandlePtr
  addDynamicContextUpdateCallback(UpdateNotificationCb callback) const override;

private:
  template <typename Value>
  std::string constructRangeParameter(absl::string_view name, Value min_value, Value max_value);

  xds::core::v3::ContextParams node_context_;
  // Map from resource type URL to dynamic context parameters.
  absl::flat_hash_map<std::string, xds::core::v3::ContextParams> dynamic_context_;
  mutable Common::CallbackManager<absl::string_view> update_cb_helper_;
};

} // namespace Config
} // namespace Envoy
