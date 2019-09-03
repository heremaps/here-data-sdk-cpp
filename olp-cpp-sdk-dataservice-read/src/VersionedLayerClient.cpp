#include <olp/dataservice/read/VersionedLayerClient.h>

#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/thread/TaskScheduler.h>

// #include "ExecuteOrSchedule.inl"

#include "ApiClientLookup.h"
#include "generated/model/Api.h"

#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
namespace {}  // end of namespace

namespace olp {
namespace dataservice {
namespace read {

VersionedLayerClient::VersionedLayerClient(
    std::shared_ptr<olp::client::OlpClientSettings> client_settings,
    std::string hrn, std::string layer_id, std::int64_t layer_version)
    : client_settings_(client_settings),
      hrn_(hrn),
      layer_id_(layer_id),
      layer_version_(layer_version) {
  // TODO: Concider move olp client as constructor argument
  // TODO: nullptr
  olp_client_ = olp::client::OlpClientFactory::Create(*client_settings);
}

olp::client::CancellationToken VersionedLayerClient::GetDataByPartitionId(
    std::string partition_id, DataResponseCallback callback) {
  ApiClientLookup::ApisResponse apis_response;

  ApiClientLookup::LookupApi(olp_client_, "query", "v1",
                             olp::client::HRN::FromString(hrn_),
                             [&](ApiClientLookup::ApisResponse response) {
                               apis_response = response;
                             });
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp