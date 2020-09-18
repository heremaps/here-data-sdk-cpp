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

namespace olp {
namespace dataservice {
namespace read {

template <typename ItemType>
using QueryItemsResult = std::map<ItemType, std::string>;

template <typename ItemType>
using QueryItemsResponse =
    ExtendedApiResponse<QueryItemsResult<ItemType>, client::ApiError,
                        client::NetworkStatistics>;

template <typename ItemType, typename QueryType>
using QueryItemsFunc = std::function<QueryItemsResponse<ItemType>(
    QueryType, client::CancellationContext)>;

template <typename ItemType>
using FilterItemsFunc =
    std::function<QueryItemsResult<ItemType>(QueryItemsResult<ItemType>)>;

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

template <typename ItemType, typename QueryType, typename PrefetchResult>
class QueryMetadataJob {
 public:
  QueryMetadataJob(
      QueryItemsFunc<ItemType, QueryType> query,
      FilterItemsFunc<ItemType> filter,
      std::shared_ptr<DownloadItemsJob<ItemType, PrefetchResult>> download_job,
      std::shared_ptr<thread::TaskScheduler> task_scheduler,
      std::shared_ptr<client::PendingRequests> pending_requests,
      client::CancellationContext execution_context, uint32_t priority)
      : query_(std::move(query)),
        filter_(std::move(filter)),
        download_job_(std::move(download_job)),
        task_scheduler_(std::move(task_scheduler)),
        pending_requests_(std::move(pending_requests)),
        execution_context_(execution_context),
        priority_(priority) {}

  void Initialize(size_t query_count) { query_count_ = query_count; }

  QueryItemsResponse<ItemType> Query(QueryType root,
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

      OLP_SDK_LOG_DEBUG_F("QueryMetadataJob", "Starting download, requests=%zu",
                          query_result_.size());

      download_job_->Initialize(query_result_.size(), accumulated_statistics_);

      auto download_job = download_job_;

      execution_context_.ExecuteOrCancelled([&]() {
        VectorOfTokens tokens;
        std::transform(
            std::begin(query_result_), std::end(query_result_),
            std::back_inserter(tokens),
            [&](const typename QueryItemsResult<ItemType>::value_type& item) {
              const std::string& data_handle = item.second;
              const auto& item_key = item.first;
              return AddTaskWithPriority(
                  task_scheduler_, pending_requests_,
                  [=](client::CancellationContext context) {
                    return download_job->Download(data_handle, context);
                  },
                  [=](ExtendedDataResponse response) {
                    download_job->CompleteItem(item_key, std::move(response));
                  },
                  priority_);
            });
        return CreateToken(std::move(tokens));
      });
    }
  }

 private:
  QueryItemsFunc<ItemType, QueryType> query_;
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
  uint32_t priority_;
  std::mutex mutex_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
