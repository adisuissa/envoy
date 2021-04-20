#include "common/config/context_provider_impl.h"

#include "common/version/api_version.h"

namespace Envoy {
namespace Config {

template <typename Value> std::string constructRangeParamValue(Value min_value, Value max_value) {
  return absl::StrCat("[", min_value, ",", max_value, "]");
}

ContextProviderImpl::ContextProviderImpl(
    const envoy::config::core::v3::Node& node,
    const Protobuf::RepeatedPtrField<std::string>& node_context_params)
    : node_context_(XdsContextParams::encodeNodeContext(node, node_context_params)) {
  // Add the range of minor versions supported by the client to the context.
  auto& params = *node_context_.mutable_params();
  params["xds.api.minor_version"] = constructRangeParamValue(
      ApiVersionInfo::oldestApiVersion().minor, ApiVersionInfo::apiVersion().minor);
}

// Config::ContextProvider
const xds::core::v3::ContextParams&
ContextProviderImpl::dynamicContext(absl::string_view resource_type_url) const {
  ASSERT(Thread::MainThread::isMainThread());
  auto it = dynamic_context_.find(resource_type_url);
  if (it != dynamic_context_.end()) {
    return it->second;
  }
  return xds::core::v3::ContextParams::default_instance();
}

void ContextProviderImpl::setDynamicContextParam(absl::string_view resource_type_url,
                                                 absl::string_view key, absl::string_view value) {
  ASSERT(Thread::MainThread::isMainThread());
  (*dynamic_context_[resource_type_url].mutable_params())[key] = value;
  update_cb_helper_.runCallbacks(resource_type_url);
}

void ContextProviderImpl::unsetDynamicContextParam(absl::string_view resource_type_url,
                                                   absl::string_view key) {
  ASSERT(Thread::MainThread::isMainThread());
  dynamic_context_[resource_type_url].mutable_params()->erase(key);
  update_cb_helper_.runCallbacks(resource_type_url);
}

ABSL_MUST_USE_RESULT Common::CallbackHandlePtr
ContextProviderImpl::addDynamicContextUpdateCallback(UpdateNotificationCb callback) const {
  ASSERT(Thread::MainThread::isMainThread());
  return update_cb_helper_.add(callback);
}

} // namespace Config
} // namespace Envoy
