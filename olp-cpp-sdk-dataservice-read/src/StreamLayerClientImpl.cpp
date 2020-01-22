/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/thread/TaskScheduler.h>
#include "ApiClientLookup.h"
#include "Common.h"
#include "generated/api/BlobApi.h"
#include "generated/api/StreamApi.h"
#include "repositories/ExecuteOrSchedule.inl"

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
      pending_requests_(std::make_shared<client::PendingRequests>()) {
  if (!settings_.cache) {
    settings_.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});
  }
}

StreamLayerClientImpl::~StreamLayerClientImpl() {
  pending_requests_->CancelAllAndWait();
}

bool StreamLayerClientImpl::CancelPendingRequests() {
  OLP_SDK_LOG_TRACE(kLogTag, "CancelPendingRequests");
  return pending_requests_->CancelAll();
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

    auto stream_api = ApiClientLookup::LookupApi(
        catalog_, context, kStreamService, kStreamVersion,
        FetchOptions::OnlineIfNotFound, settings_);

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
          subscripton_id, subscription_mode,
          subscription.GetResult().GetNodeBaseURL(), correlation_id);
    }

    OLP_SDK_LOG_INFO_F(kLogTag,
                       "Subscribe: done, subscription_id=%s, node_base_url=%s, "
                       "correlation_id=%s",
                       subscripton_id.c_str(),
                       subscription.GetResult().GetNodeBaseURL().c_str(),
                       correlation_id.c_str());

    return subscripton_id;
  };

  return AddTask(settings_.task_scheduler, pending_requests_,
                 std::move(subscribe_task), std::move(callback));
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
    std::string node_base_url;
    std::string x_correlation_id;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!client_context_) {
        OLP_SDK_LOG_WARNING_F(kLogTag,
                              "Unubscribe: unsuccessful, not subscribed");

        return client::ApiError(client::ErrorCode::PreconditionFailed,
                                "Not subscribed", false);
      }

      subscription_id = client_context_->subscription_id;
      subscription_mode = client_context_->subscription_mode;
      node_base_url = client_context_->node_base_url;
      x_correlation_id = client_context_->x_correlation_id;
    }

    OLP_SDK_LOG_INFO_F(
        kLogTag,
        "Unsubscribe: started, subscription_id=%s, subscription_mode=%s, "
        "node_base_url=%s, x_correlation_id=%s",
        subscription_id.c_str(), subscription_mode.c_str(),
        node_base_url.c_str(), x_correlation_id.c_str());

    client::OlpClient client;
    client.SetBaseUrl(node_base_url);
    client.SetSettings(settings_);

    const auto response = StreamApi::DeleteSubscription(
        client, layer_id_, subscription_id, subscription_mode, x_correlation_id,
        context);

    if (!response.IsSuccessful()) {
      OLP_SDK_LOG_WARNING_F(kLogTag, "Unsubscribe: unsuccessful, error=%s",
                            response.GetError().GetMessage().c_str());
      return response.GetError();
    }

    {
      std::lock_guard<std::mutex> lock(mutex_);
      client_context_.release();
    }

    OLP_SDK_LOG_INFO_F(kLogTag, "Unsubscribe: done, subscription_id=%s",
                       subscription_id.c_str());

    return subscription_id;
  };

  return AddTask(settings_.task_scheduler, pending_requests_,
                 std::move(unsubscribe_task), std::move(callback));
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
  const auto& data_handle = message.GetMetaData().GetDataHandle();
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

    auto blob_api = ApiClientLookup::LookupApi(
        catalog_, context, kBlobService, kBlobVersion,
        FetchOptions::OnlineIfNotFound, settings_);

    if (!blob_api.IsSuccessful()) {
      return blob_api.GetError();
    }

    const auto blob_response =
        BlobApi::GetBlob(blob_api.GetResult(), layer_id_, data_handle.value(),
                         boost::none, boost::none, context);

    OLP_SDK_LOG_INFO_F(kLogTag,
                       "GetData: done, blob_response is successful: %s",
                       blob_response.IsSuccessful() ? "true" : "false");

    return blob_response;
  };

  return AddTask(settings_.task_scheduler, pending_requests_,
                 std::move(get_data_task), std::move(callback));
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

}  // namespace read
}  // namespace dataservice
}  // namespace olp
