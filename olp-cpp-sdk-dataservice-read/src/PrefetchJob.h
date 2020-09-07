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
#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/PendingRequests.h>
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

template <typename ItemType>
using QueryItemsResult = std::map<ItemType, std::string>;

template <typename ItemType>
using QueryItemsResponse =
    ExtendedApiResponse<QueryItemsResult<ItemType>, client::ApiError,
                        client::NetworkStatistics>;

template <typename ItemType>
using QueryItemsFunc = std::function<QueryItemsResponse<ItemType>(
    ItemType, client::CancellationContext)>;

template <typename ItemType>
using FilterItemsFunc =
    std::function<QueryItemsResult<ItemType>(QueryItemsResult<ItemType>)>;

// Prototype of function used to download data using data handle.
using DownloadFunc = std::function<ExtendedDataResponse(
    std::string, client::CancellationContext)>;

template <typename PrefetchItemsResult>
using PrefetchItemsResponseCallback = Callback<PrefetchItemsResult>;

class TokenHelper {
 public:
  using VectorOfTokens = std::vector<olp::client::CancellationToken>;

  static olp::client::CancellationToken CreateToken(VectorOfTokens tokens) {
    return olp::client::CancellationToken(std::bind(
        [](const VectorOfTokens& tokens) {
          std::for_each(std::begin(tokens), std::end(tokens),
                        [](const olp::client::CancellationToken& token) {
                          token.Cancel();
                        });
        },
        std::move(tokens)));
  }
};
template <typename ItemType, typename PrefetchResult>
class DownloadItemsJob {
 public:
  DownloadItemsJob(DownloadFunc download,
                   Callback<PrefetchResult> user_callback,
                   PrefetchStatusCallback status_callback)
      : download_(std::move(download)),
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
      prefetch_result_.push_back(
          std::make_shared<typename PrefetchResult::value_type::element_type>(
              item,
              typename PrefetchResult::value_type::element_type::ResultType()));
      requests_succeeded_++;
    } else {
      prefetch_result_.push_back(
          std::make_shared<typename PrefetchResult::value_type::element_type>(
              item, response.GetError()));
      requests_failed_++;
    }

    if (status_callback_) {
      status_callback_(
          PrefetchStatus{prefetch_result_.size(), total_download_task_count_,
                         GetAccumulatedBytes(accumulated_statistics_)});
    }

    if (!--download_task_count_) {
      OLP_SDK_LOG_DEBUG_F("PrefetchJob",
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
  Callback<PrefetchResult> user_callback_;
  PrefetchStatusCallback status_callback_;

  size_t download_task_count_{0};
  size_t total_download_task_count_{0};

  size_t requests_succeeded_{0};
  size_t requests_failed_{0};

  client::NetworkStatistics accumulated_statistics_;

  PrefetchResult prefetch_result_;

  std::mutex mutex_;
};

template <typename ItemType, typename PrefetchResult>
class QueryMetadataJob {
 public:
  QueryMetadataJob(
      QueryItemsFunc<ItemType> query, FilterItemsFunc<ItemType> filter,
      std::shared_ptr<DownloadItemsJob<ItemType, PrefetchResult>> download_job,
      std::shared_ptr<thread::TaskScheduler> task_scheduler,
      std::shared_ptr<client::PendingRequests> pending_requests,
      client::CancellationContext execution_context)
      : query_(std::move(query)),
        filter_(std::move(filter)),
        download_job_(std::move(download_job)),
        task_scheduler_(std::move(task_scheduler)),
        pending_requests_(std::move(pending_requests)),
        execution_context_(execution_context) {}

  void Initialize(size_t query_count) { query_count_ = query_count; }

  QueryItemsResponse<ItemType> Query(ItemType root,
                                     client::CancellationContext context) {
    return query_(root, context);
  }

  void CompleteQuery(QueryItemsResponse<ItemType> response) {
    std::lock_guard<std::mutex> lock(mutex_);

    accumulated_statistics_ += GetNetworkStatistics(response);

    if (response.IsSuccessful()) {
      auto items = response.MoveResult();
      query_result_.insert(std::make_move_iterator(items.begin()),
                           std::make_move_iterator(items.end()));
    } else {
      const auto& error = response.GetError();
      if (error.GetErrorCode() == client::ErrorCode::Cancelled) {
        canceled_ = true;
      } else {
        // The old behavior: when one of the query requests fails, we fail the
        // entire prefetch.
        query_error_ = error;
      }
    }

    if (!--query_count_) {
      if (query_error_) {
        download_job_->OnPrefetchCompleted(query_error_.value());
        return;
      }

      if (canceled_) {
        download_job_->OnPrefetchCompleted(
            {{client::ErrorCode::Cancelled, "Cancelled"}});
        return;
      }

      if (filter_) {
        query_result_ = filter_(std::move(query_result_));
      }

      if (query_result_.empty()) {
        download_job_->OnPrefetchCompleted(PrefetchResult());
        return;
      }

      OLP_SDK_LOG_DEBUG_F("PrefetchJob", "Starting download, requests=%zu",
                          query_result_.size());

      download_job_->Initialize(query_result_.size(), accumulated_statistics_);

      auto download_job = download_job_;

      execution_context_.ExecuteOrCancelled([&]() {
        TokenHelper::VectorOfTokens tokens;
        std::transform(
            std::begin(query_result_), std::end(query_result_),
            std::back_inserter(tokens),
            [&](const typename QueryItemsResult<ItemType>::value_type& item) {
              const std::string& data_handle = item.second;
              const auto& item_key = item.first;
              return AddTask(
                  task_scheduler_, pending_requests_,
                  [=](client::CancellationContext context) {
                    return download_job->Download(data_handle, context);
                  },
                  [=](ExtendedDataResponse response) {
                    download_job->CompleteItem(item_key, std::move(response));
                  });
            });
        return TokenHelper::CreateToken(std::move(tokens));
      });
    }
  }

 private:
  QueryItemsFunc<ItemType> query_;
  FilterItemsFunc<ItemType> filter_;
  size_t query_count_{0};

  bool canceled_{false};

  QueryItemsResult<ItemType> query_result_;

  client::NetworkStatistics accumulated_statistics_;

  boost::optional<client::ApiError> query_error_;

  std::shared_ptr<DownloadItemsJob<ItemType, PrefetchResult>> download_job_;
  std::shared_ptr<thread::TaskScheduler> task_scheduler_;
  std::shared_ptr<client::PendingRequests> pending_requests_;
  client::CancellationContext execution_context_;

  std::mutex mutex_;
};

class PrefetchHelper {
 public:
  template <typename ItemType, typename PrefetchResult>
  static client::CancellationToken Prefetch(
      const std::vector<ItemType>& roots, QueryItemsFunc<ItemType> query,
      FilterItemsFunc<ItemType> filter, DownloadFunc download,
      Callback<PrefetchResult> user_callback,
      PrefetchStatusCallback status_callback,
      std::shared_ptr<thread::TaskScheduler> task_scheduler,
      std::shared_ptr<client::PendingRequests> pending_requests) {
    client::CancellationContext execution_context;

    auto download_job =
        std::make_shared<DownloadItemsJob<ItemType, PrefetchResult>>(
            std::move(download), std::move(user_callback),
            std::move(status_callback));

    auto query_job =
        std::make_shared<QueryMetadataJob<ItemType, PrefetchResult>>(
            std::move(query), std::move(filter), download_job, task_scheduler,
            pending_requests, execution_context);

    query_job->Initialize(roots.size());

    OLP_SDK_LOG_DEBUG_F("PrefetchJob", "Starting queries, requests=%zu",
                        roots.size());

    execution_context.ExecuteOrCancelled([&]() {
      TokenHelper::VectorOfTokens tokens;
      std::transform(std::begin(roots), std::end(roots),
                     std::back_inserter(tokens), [&](ItemType root) {
                       return AddTask(
                           task_scheduler, pending_requests,
                           [=](client::CancellationContext context) {
                             return query_job->Query(root, context);
                           },
                           [=](QueryItemsResponse<ItemType> response) {
                             query_job->CompleteQuery(std::move(response));
                           });
                     });
      return TokenHelper::CreateToken(std::move(tokens));
    });

    return client::CancellationToken(
        [execution_context]() mutable { execution_context.CancelOperation(); });
  }
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
