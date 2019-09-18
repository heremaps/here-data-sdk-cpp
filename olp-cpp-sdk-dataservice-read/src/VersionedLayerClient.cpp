#include <olp/dataservice/read/VersionedLayerClient.h>

#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/thread/TaskScheduler.h>

#include "ApiClientLookup.h"
#include "generated/model/Api.h"

#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/context/Context.h>

#include "generated/api/BlobApi.h"
#include "generated/api/MetadataApi.h"

#include <algorithm>

namespace {

class Condition {
 public:
  Condition(olp::client::CancellationContext context, int timeout = 60)
      : context_{context}, timeout_{timeout}, signaled_{false} {}

  void Notify() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      signaled_ = true;
    }
    condition_.notify_one();
  }

  bool Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    bool triggered = condition_.wait_for(
        lock, std::chrono::seconds(timeout_),
        [&] { return signaled_ || context_.IsCancelled(); });

    signaled_ = false;
    return triggered;
  }

 private:
  olp::client::CancellationContext context_;
  int timeout_;
  std::condition_variable condition_;
  std::mutex mutex_;
  bool signaled_;
};

}  // end of namespace

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
  olp::client::CancellationContext context;
  olp::client::CancellationToken token(
      [=]() mutable { context.CancelOperation(); });
  Condition condition(context);
  auto wait_and_check = [&] {
    if (!condition.Wait() && !context.IsCancelled()) {
      callback({{olp::client::ErrorCode::RequestTimeout,
                 "Network request timed out.", true}});
      return false;
    }
    return true;
  };

  // Step 1.

  ApiClientLookup::ApiClientResponse apis_response;

  context.ExecuteOrCancelled([&]() {
    return ApiClientLookup::LookupApiClient(
        olp_client_, "query", "v1", olp::client::HRN::FromString(hrn_),
        [&](ApiClientLookup::ApiClientResponse response) {
          apis_response = std::move(response);
          condition.Notify();
        });
  });

  if (!apis_response.IsSuccessful() || !wait_and_check()) {
    return token;
  }

  auto query_client = apis_response.GetResult();

  // Step 2.

  MetadataApi::PartitionsResponse partitions_response;
  context.ExecuteOrCancelled([&]() {
    return olp::dataservice::read::MetadataApi::GetPartitions(
        query_client, layer_id_, layer_version_, boost::none, boost::none,
        boost::none, [&](MetadataApi::PartitionsResponse response) {
          partitions_response = std::move(response);
          condition.Notify();
        });
  });

  if (!partitions_response.IsSuccessful() || !wait_and_check()) {
    return token;
  }

  auto partitions = partitions_response.GetResult();

  // Step 3.

  context.ExecuteOrCancelled([&]() {
    return ApiClientLookup::LookupApiClient(
        olp_client_, "blob", "v1", olp::client::HRN::FromString(hrn_),
        [&](ApiClientLookup::ApiClientResponse response) {
          apis_response = std::move(response);
          condition.Notify();
        });
  });

  if (!apis_response.IsSuccessful() || !wait_and_check()) {
    return token;
  }

  auto blob_client = apis_response.GetResult();

  // Step 4.
  // TODO: check partitions is not empty

  auto partition_it = std::find_if(partitions.GetPartitions().begin(),
                                   partitions.GetPartitions().end(),
                                   [&](const model::Partition& p) {
                                     return p.GetPartition() == partition_id;
                                   });
  if (partition_it == partitions.GetPartitions().end()) {
    return token;
  }
  auto data_handle = partition_it->GetDataHandle();
  BlobApi::DataResponse data_response;
  context.ExecuteOrCancelled([&]() {
    return olp::dataservice::read::BlobApi::GetBlob(
        blob_client, layer_id_, data_handle, boost::none, boost::none,
        [&](BlobApi::DataResponse response) {
          data_response = std::move(response);
          condition.Notify();
        });
  });

  if (!data_response.IsSuccessful() || !wait_and_check()) {
    return token;
  }

  auto data = data_response.GetResult();

  callback(data);

  return token;
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp