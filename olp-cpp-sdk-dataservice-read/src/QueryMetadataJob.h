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

#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/client/CancellationContext.h>
#include <olp/core/logging/Log.h>
#include <olp/dataservice/read/Types.h>
#include "Common.h"
#include "ExtendedApiResponse.h"
#include "TaskSink.h"

namespace olp {
namespace dataservice {
namespace read {

template <typename ItemType, typename QueryType, typename QueryResponseType>
using QueryItemsFunc =
    std::function<QueryResponseType(QueryType, client::CancellationContext)>;

template <typename QueryResponseType>
using FilterItemsFunc = std::function<QueryResponseType(QueryResponseType)>;

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

template <typename ItemType, typename QueryType, typename PrefetchResult,
          typename QueryResponseType, typename PrefetchStatusType>
class QueryMetadataJob {
 public:
  QueryMetadataJob(
      QueryItemsFunc<ItemType, QueryType, QueryResponseType> query,
      FilterItemsFunc<typename QueryResponseType::ResultType> filter,
      std::shared_ptr<
          DownloadItemsJob<ItemType, PrefetchResult, PrefetchStatusType>>
          download_job,
      TaskSink& task_sink, client::CancellationContext execution_context,
      uint32_t priority)
      : query_(std::move(query)),
        filter_(std::move(filter)),
        download_job_(std::move(download_job)),
        task_sink_(task_sink),
        execution_context_(execution_context),
        priority_(priority) {}

  virtual ~QueryMetadataJob() = default;

  virtual bool CheckIfFail() {
    // The old behavior: when one of the query requests fails, we fail the
    // entire prefetch.
    return (!query_errors_.empty());
  }

  void Initialize(size_t query_count) {
    query_count_ = query_count;
    query_size_ = query_count;
  }

  QueryResponseType Query(QueryType root, client::CancellationContext context) {
    return query_(root, context);
  }

  void CompleteQuery(QueryResponseType response) {
    std::lock_guard<std::mutex> lock(mutex_);

    accumulated_statistics_ += GetNetworkStatistics(response);

    if (response.IsSuccessful()) {
      auto items = response.MoveResult();
      std::move(items.begin(), items.end(),
                std::inserter(query_result_, query_result_.begin()));
    } else {
      const auto& error = response.GetError();
      if (error.GetErrorCode() == client::ErrorCode::Cancelled) {
        canceled_ = true;
      } else {
        // Collect all errors.
        query_errors_.push_back(error);
      }
    }

    if (!--query_count_) {
      if (CheckIfFail()) {
        download_job_->OnPrefetchCompleted(query_errors_.front());
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

      bool all_download_tasks_triggered = true;

      execution_context_.ExecuteOrCancelled([&]() {
        VectorOfTokens tokens;
        std::transform(
            std::begin(query_result_), std::end(query_result_),
            std::back_inserter(tokens),
            [&](const typename QueryResponseType::ResultType::value_type&
                    item) {
              const std::string& data_handle = item.second;
              const auto& item_key = item.first;

              auto result = task_sink_.AddTaskChecked(
                  [=](client::CancellationContext context) {
                    return download_job->Download(data_handle, context);
                  },
                  [=](ExtendedDataResponse response) {
                    download_job->CompleteItem(item_key, std::move(response));
                  },
                  priority_);

              if (result) {
                return *result;
              }

              all_download_tasks_triggered = false;
              return client::CancellationToken();
            });
        return CreateToken(std::move(tokens));
      });

      if (!all_download_tasks_triggered) {
        execution_context_.CancelOperation();
        download_job_->OnPrefetchCompleted(
            {{client::ErrorCode::Cancelled, "Cancelled"}});
      }
    }
  }

 protected:
  QueryItemsFunc<ItemType, QueryType, QueryResponseType> query_;
  FilterItemsFunc<typename QueryResponseType::ResultType> filter_;
  size_t query_count_{0};
  size_t query_size_{0};
  bool canceled_{false};
  typename QueryResponseType::ResultType query_result_;
  client::NetworkStatistics accumulated_statistics_;
  std::vector<client::ApiError> query_errors_;
  std::shared_ptr<
      DownloadItemsJob<ItemType, PrefetchResult, PrefetchStatusType>>
      download_job_;
  TaskSink& task_sink_;
  client::CancellationContext execution_context_;
  uint32_t priority_;
  std::mutex mutex_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
