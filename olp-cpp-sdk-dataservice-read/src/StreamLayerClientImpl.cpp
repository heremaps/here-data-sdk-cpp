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
                            stream_api.GetError().GetMessage().c_str());
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

}  // namespace read
}  // namespace dataservice
}  // namespace olp
