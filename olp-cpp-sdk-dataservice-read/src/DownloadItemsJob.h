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

#pragma once

#include <olp/core/client/CancellationContext.h>
#include <olp/core/logging/Log.h>
#include <olp/dataservice/read/Types.h>
#include "Common.h"
#include "ExtendedApiResponse.h"
#include "ExtendedApiResponseHelpers.h"

namespace olp {
namespace dataservice {
namespace read {

using ExtendedDataResponse = ExtendedApiResponse<model::Data, client::ApiError,
                                                 client::NetworkStatistics>;

// Prototype of function used to download data using data handle.
using DownloadFunc = std::function<ExtendedDataResponse(
    std::string, client::CancellationContext)>;

template <typename ItemType, typename PrefetchResult>
using AppendResultFunc =
    std::function<void(ExtendedDataResponse response, ItemType item,
                       PrefetchResult& prefetch_result)>;

template <typename PrefetchStatusType>
using PrefetchStatusCallbackType = std::function<void(PrefetchStatusType)>;

template <typename ItemType, typename PrefetchResult,
          typename PrefetchStatusType>
class DownloadItemsJob {
 public:
  DownloadItemsJob(
      DownloadFunc download,
      AppendResultFunc<ItemType, PrefetchResult> append_result,
      Callback<PrefetchResult> user_callback,
      PrefetchStatusCallbackType<PrefetchStatusType> status_callback)
      : download_(std::move(download)),
        append_result_(std::move(append_result)),
        user_callback_(std::move(user_callback)),
        status_callback_(std::move(status_callback)) {}

  void Initialize(size_t items_count, client::NetworkStatistics statistics) {
    download_task_count_ = total_download_task_count_ = items_count;
    accumulated_statistics_ = statistics;
  }

  ExtendedDataResponse Download(const std::string& data_handle,
                                client::CancellationContext context) {
    return download_(data_handle, context);
  }
  size_t GetAccumulatedBytes(const olp::client::NetworkStatistics& statistics) {
    // This narrow cast is necessary to avoid narrowing compiler errors like
    // -Wc++11-narrowing when building for 32bit targets.
    const auto bytes_transferred =
        statistics.GetBytesDownloaded() + statistics.GetBytesUploaded();
    return static_cast<size_t>(bytes_transferred &
                               std::numeric_limits<size_t>::max());
  }

  void CompleteItem(ItemType item, ExtendedDataResponse response) {
    std::lock_guard<std::mutex> lock(mutex_);
    accumulated_statistics_ += GetNetworkStatistics(response);

    if (response.IsSuccessful()) {
      requests_succeeded_++;
    } else {
      if (response.GetError().GetErrorCode() ==
              olp::client::ErrorCode::Cancelled &&
          user_callback_) {
        auto user_callback = std::move(user_callback_);
        if (user_callback) {
          user_callback(client::ApiError::Cancelled());
        }
        return;
      }

      requests_failed_++;
    }

    append_result_(response, item, prefetch_result_);

    if (status_callback_) {
      status_callback_(PrefetchStatusType{
          requests_succeeded_ + requests_failed_, total_download_task_count_,
          GetAccumulatedBytes(accumulated_statistics_)});
    }

    if (!--download_task_count_ && user_callback_) {
      OLP_SDK_LOG_DEBUG_F("DownloadItemsJob",
                          "Download complete, succeeded=%zu, failed=%zu",
                          requests_succeeded_, requests_failed_);

      OnPrefetchCompleted(std::move(prefetch_result_));
    }
  }

  void OnPrefetchCompleted(Response<PrefetchResult> result) {
    auto prefetch_callback = std::move(user_callback_);
    prefetch_callback(std::move(result));
  }

 private:
  DownloadFunc download_;
  AppendResultFunc<ItemType, PrefetchResult> append_result_;
  Callback<PrefetchResult> user_callback_;
  PrefetchStatusCallbackType<PrefetchStatusType> status_callback_;
  size_t download_task_count_{0};
  size_t total_download_task_count_{0};
  size_t requests_succeeded_{0};
  size_t requests_failed_{0};
  client::NetworkStatistics accumulated_statistics_;
  PrefetchResult prefetch_result_;
  std::mutex mutex_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
