/*
 * Copyright (C) 2020-2024 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

#include "StreamLayerClientImpl.h"

#include <algorithm>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/thread/TaskScheduler.h>
#include "Common.h"
#include "generated/api/BlobApi.h"
#include "generated/api/StreamApi.h"

namespace olp {
namespace dataservice {
namespace read {

namespace {
constexpr auto kLogTag = "StreamLayerClientImpl";
constexpr auto kSubscriptionModeParallel = "parallel";
constexpr auto kSubscriptionModeSerial = "serial";
constexpr auto kStreamService = "stream";
constexpr auto kStreamVersion = "v2";
constexpr auto kBlobService = "blob";
constexpr auto kBlobVersion = "v1";

std::string GetSubscriptionMode(SubscribeRequest::SubscriptionMode mode) {
  return mode == SubscribeRequest::SubscriptionMode::kSerial
             ? kSubscriptionModeSerial
             : kSubscriptionModeParallel;
}

}  // namespace

StreamLayerClientImpl::StreamLayerClientImpl(client::HRN catalog,
                                             std::string layer_id,
                                             client::OlpClientSettings settings)
    : catalog_(std::move(catalog)),
      layer_id_(std::move(layer_id)),
      settings_(std::move(settings)),
      lookup_client_(catalog_, settings_),
      task_sink_(settings_.task_scheduler) {
  if (!settings_.cache) {
    settings_.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});
  }
}

StreamLayerClientImpl::~StreamLayerClientImpl() = default;

bool StreamLayerClientImpl::CancelPendingRequests() {
  OLP_SDK_LOG_TRACE(kLogTag, "CancelPendingRequests");
  task_sink_.CancelTasks();
  return true;
}

client::CancellationToken StreamLayerClientImpl::Subscribe(
    SubscribeRequest request, SubscribeResponseCallback callback) {
  auto subscribe_task =
      [=](client::CancellationContext context) -> SubscribeResponse {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (client_context_) {
        OLP_SDK_LOG_WARNING_F(
            kLogTag, "Subscribe: already subscribed, subscription_id=%s",
            client_context_->subscription_id.c_str());

        return client::ApiError(client::ErrorCode::InvalidArgument,
                                "Already subscribed", false);
      }
    }

    const auto subscription_mode =
        GetSubscriptionMode(request.GetSubscriptionMode());

    OLP_SDK_LOG_INFO_F(
        kLogTag,
        "Subscribe: started, subscription_id=%s, consumer_id=%s, "
        "subscription_mode=%s",
        request.GetSubscriptionId().get_value_or("none").c_str(),
        request.GetConsumerId().get_value_or("none").c_str(),
        subscription_mode.c_str());

    auto stream_api = lookup_client_.LookupApi(
        kStreamService, kStreamVersion, client::OnlineIfNotFound, context);

    if (!stream_api.IsSuccessful()) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag, "Subscribe: unsuccessful Stream API lookup, error=%s",
          stream_api.GetError().GetMessage().c_str());
      return stream_api.GetError();
    }

    std::string correlation_id;
    const auto subscription = StreamApi::Subscribe(
        stream_api.GetResult(), layer_id_, request.GetSubscriptionId(),
        subscription_mode, request.GetConsumerId(),
        request.GetConsumerProperties(), context, correlation_id);

    if (!subscription.IsSuccessful()) {
      OLP_SDK_LOG_WARNING_F(kLogTag, "Subscribe: unsuccessful, error=%s",
                            subscription.GetError().GetMessage().c_str());
      return subscription.GetError();
    }

    const auto subscripton_id = subscription.GetResult().GetSubscriptionId();
    {
      std::lock_guard<std::mutex> lock(mutex_);

      client_context_ = std::make_unique<StreamLayerClientContext>(
          subscripton_id, subscription_mode, correlation_id,
          std::make_shared<client::OlpClient>(
              settings_, subscription.GetResult().GetNodeBaseURL()));
    }

    OLP_SDK_LOG_INFO_F(kLogTag,
                       "Subscribe: done, subscription_id=%s, node_base_url=%s, "
                       "correlation_id=%s",
                       subscripton_id.c_str(),
                       subscription.GetResult().GetNodeBaseURL().c_str(),
                       correlation_id.c_str());

    return subscripton_id;
  };

  return task_sink_.AddTask(std::move(subscribe_task), std::move(callback),
                            thread::NORMAL);
}

client::CancellableFuture<SubscribeResponse> StreamLayerClientImpl::Subscribe(
    SubscribeRequest request) {
  auto promise = std::make_shared<std::promise<SubscribeResponse>>();
  auto cancel_token =
      Subscribe(std::move(request), [promise](SubscribeResponse response) {
        promise->set_value(std::move(response));
      });

  return olp::client::CancellableFuture<SubscribeResponse>(
      std::move(cancel_token), std::move(promise));
}

client::CancellationToken StreamLayerClientImpl::Unsubscribe(
    UnsubscribeResponseCallback callback) {
  auto unsubscribe_task =
      [=](client::CancellationContext context) -> UnsubscribeResponse {
    std::string subscription_id;
    std::string subscription_mode;
    std::string x_correlation_id;
    std::shared_ptr<client::OlpClient> client;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!client_context_) {
        OLP_SDK_LOG_WARNING_F(
            kLogTag, "Unsubscribe: unsuccessful, subscription missing");

        return client::ApiError(client::ErrorCode::PreconditionFailed,
                                "Subscription missing", false);
      }

      subscription_id = client_context_->subscription_id;
      subscription_mode = client_context_->subscription_mode;
      x_correlation_id = client_context_->x_correlation_id;
      client = client_context_->client;
    }

    OLP_SDK_LOG_INFO_F(kLogTag,
                       "Unsubscribe: started, subscription_id=%s, "
                       "subscription_mode=%s, x_correlation_id=%s",
                       subscription_id.c_str(), subscription_mode.c_str(),
                       x_correlation_id.c_str());

    const auto response = StreamApi::DeleteSubscription(
        *client, layer_id_, subscription_id, subscription_mode,
        x_correlation_id, context);

    if (!response.IsSuccessful()) {
      OLP_SDK_LOG_WARNING_F(kLogTag, "Unsubscribe: unsuccessful, error=%s",
                            response.GetError().GetMessage().c_str());
      return response.GetError();
    }

    {
      std::lock_guard<std::mutex> lock(mutex_);
      client_context_.reset();
    }

    OLP_SDK_LOG_INFO_F(kLogTag, "Unsubscribe: done, subscription_id=%s",
                       subscription_id.c_str());

    return subscription_id;
  };

  return task_sink_.AddTask(std::move(unsubscribe_task), std::move(callback),
                            thread::NORMAL);
}

client::CancellableFuture<UnsubscribeResponse>
StreamLayerClientImpl::Unsubscribe() {
  auto promise = std::make_shared<std::promise<UnsubscribeResponse>>();
  auto cancel_token = Unsubscribe([promise](UnsubscribeResponse response) {
    promise->set_value(std::move(response));
  });

  return olp::client::CancellableFuture<UnsubscribeResponse>(
      std::move(cancel_token), std::move(promise));
}

client::CancellationToken StreamLayerClientImpl::GetData(
    const model::Message& message, DataResponseCallback callback) {
  const auto& metadata = message.GetMetaData();

  const auto& data_handle = metadata.GetDataHandle();
  const auto data_size = metadata.GetDataSize();
  auto get_data_task =
      [=](client::CancellationContext context) -> DataResponse {
    if (!data_handle) {
      OLP_SDK_LOG_WARNING(kLogTag,
                          "GetData: message does not contain data handle");
      return client::ApiError(client::ErrorCode::InvalidArgument,
                              "Data handle is missing in the message metadata. "
                              "Please use embedded message data directly.");
    }

    OLP_SDK_LOG_INFO_F(kLogTag, "GetData: started, data_handle=%s",
                       data_handle.value().c_str());

    auto blob_api = lookup_client_.LookupApi(kBlobService, kBlobVersion,
                                             client::OnlineIfNotFound, context);

    if (!blob_api.IsSuccessful()) {
      return blob_api.GetError();
    }

    model::Partition partition;
    partition.SetDataHandle(*data_handle);
    partition.SetDataSize(data_size);

    const auto blob_response =
        BlobApi::GetBlob(blob_api.GetResult(), layer_id_, partition,
                         boost::none, boost::none, context);

    OLP_SDK_LOG_INFO_F(kLogTag,
                       "GetData: done, blob_response is successful: %s",
                       blob_response.IsSuccessful() ? "true" : "false");

    return blob_response;
  };

  return task_sink_.AddTask(std::move(get_data_task), std::move(callback),
                            thread::NORMAL);
}

client::CancellableFuture<DataResponse> StreamLayerClientImpl::GetData(
    const model::Message& message) {
  auto promise = std::make_shared<std::promise<DataResponse>>();
  auto cancel_token = GetData(message, [promise](DataResponse response) {
    promise->set_value(std::move(response));
  });

  return olp::client::CancellableFuture<DataResponse>(std::move(cancel_token),
                                                      std::move(promise));
}

client::CancellationToken StreamLayerClientImpl::Poll(
    PollResponseCallback callback) {
  auto poll_task = [=](client::CancellationContext context) -> PollResponse {
    std::string subscription_id;
    std::string subscription_mode;
    std::string x_correlation_id;
    std::shared_ptr<client::OlpClient> client;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!client_context_) {
        OLP_SDK_LOG_WARNING_F(kLogTag,
                              "Poll: unsuccessful, subscription missing");

        return client::ApiError(client::ErrorCode::PreconditionFailed,
                                "Subscription missing", false);
      }

      subscription_id = client_context_->subscription_id;
      subscription_mode = client_context_->subscription_mode;
      x_correlation_id = client_context_->x_correlation_id;
      client = client_context_->client;
    }

    OLP_SDK_LOG_INFO_F(kLogTag,
                       "Poll: started, subscription_id=%s, "
                       "subscription_mode=%s, x_correlation_id=%s",
                       subscription_id.c_str(), subscription_mode.c_str(),
                       x_correlation_id.c_str());

    auto data =
        StreamApi::ConsumeData(*client, layer_id_, subscription_id,
                               subscription_mode, context, x_correlation_id);

    if (!data.IsSuccessful()) {
      OLP_SDK_LOG_WARNING_F(kLogTag, "Poll: couldn't consume data, error=%s",
                            data.GetError().GetMessage().c_str());
      return data.GetError();
    }

    auto res = data.MoveResult();

    const auto& messages = res.GetMessages();

    if (messages.empty()) {
      OLP_SDK_LOG_INFO_F(kLogTag, "Poll: done, no new messages received.");
      return res;
    }

    // Get offsets for all partitions presented in messages.
    struct Compare {
      bool operator()(const model::StreamOffset& lhs,
                      const model::StreamOffset& rhs) const {
        return lhs.GetPartition() < rhs.GetPartition();
      }
    };

    std::set<model::StreamOffset, Compare> stream_offsets;

    std::transform(messages.rbegin(), messages.rend(),
                   std::inserter(stream_offsets, stream_offsets.end()),
                   [](const model::Message& msg) { return msg.GetOffset(); });

    // Commit offsets
    model::StreamOffsets offsets_request;
    offsets_request.SetOffsets(std::vector<model::StreamOffset>(
        stream_offsets.begin(), stream_offsets.end()));
    auto commit_res = StreamApi::CommitOffsets(
        *client, layer_id_, offsets_request, subscription_id, subscription_mode,
        context, x_correlation_id);

    if (!commit_res.IsSuccessful()) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "Poll: commit offsets unsuccessful, error=%s",
                            commit_res.GetError().GetMessage().c_str());
      return commit_res.GetError();
    }
    OLP_SDK_LOG_INFO_F(kLogTag, "Poll: done, response is successful.");

    return res;
  };

  return task_sink_.AddTask(std::move(poll_task), std::move(callback),
                            thread::NORMAL);
}

client::CancellableFuture<PollResponse> StreamLayerClientImpl::Poll() {
  auto promise = std::make_shared<std::promise<PollResponse>>();
  auto cancel_token = Poll([promise](PollResponse response) {
    promise->set_value(std::move(response));
  });

  return olp::client::CancellableFuture<PollResponse>(std::move(cancel_token),
                                                      std::move(promise));
}

client::CancellationToken StreamLayerClientImpl::Seek(
    SeekRequest request, SeekResponseCallback callback) {
  auto seek_task = [=](client::CancellationContext context) -> SeekResponse {
    std::string subscription_id;
    std::string subscription_mode;
    std::string x_correlation_id;
    std::shared_ptr<client::OlpClient> client;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!client_context_) {
        OLP_SDK_LOG_WARNING_F(kLogTag,
                              "Seek: unsuccessful, subscription missing");
        return client::ApiError(client::ErrorCode::PreconditionFailed,
                                "Subscription missing", false);
      }

      subscription_id = client_context_->subscription_id;
      subscription_mode = client_context_->subscription_mode;
      x_correlation_id = client_context_->x_correlation_id;
      client = client_context_->client;
    }

    auto const& offsets = request.GetOffsets();
    if (offsets.GetOffsets().empty()) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "Seek: unsuccessful, stream offsets missing");
      return client::ApiError(client::ErrorCode::PreconditionFailed,
                              "Stream offsets missing", false);
    }

    auto res =
        StreamApi::SeekToOffset(*client, layer_id_, offsets, subscription_id,
                                subscription_mode, context, x_correlation_id);

    if (!res.IsSuccessful()) {
      OLP_SDK_LOG_WARNING_F(kLogTag,
                            "Seek: seek offsets unsuccessful, error=%s",
                            res.GetError().GetMessage().c_str());
      return res.GetError();
    }
    OLP_SDK_LOG_INFO_F(kLogTag, "Seek: done, response is successful.");

    return res;
  };

  return task_sink_.AddTask(std::move(seek_task), std::move(callback),
                            thread::NORMAL);
}
client::CancellableFuture<SeekResponse> StreamLayerClientImpl::Seek(
    SeekRequest request) {
  auto promise = std::make_shared<std::promise<SeekResponse>>();
  auto cancel_token = Seek(request, [promise](SeekResponse response) {
    promise->set_value(std::move(response));
  });

  return olp::client::CancellableFuture<SeekResponse>(std::move(cancel_token),
                                                      std::move(promise));
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
