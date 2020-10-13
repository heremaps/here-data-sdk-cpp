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

#include <olp/core/client/ApiNoResult.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/logging/Log.h>
#include <olp/dataservice/read/Types.h>
#include "Common.h"
#include "ExtendedApiResponse.h"
#include "TaskSink.h"

namespace olp {
namespace dataservice {
namespace read {
using EmptyResponse = Response<client::ApiNoResult>;

template <typename QueryType, typename QueryResult>
struct QueryResponse {
  QueryResponse() = default;
  explicit QueryResponse(QueryType root) : root(std::move(root)) {}
  QueryType root;
  EmptyResponse response;
  QueryResult list_of_tiles;
};

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
  using QueryResult = typename QueryResponseType::ResultType;
  using QueryResultItem = typename QueryResponseType::ResultType::value_type;

  QueryMetadataJob(
      QueryItemsFunc<ItemType, QueryType, QueryResponseType> query,
      FilterItemsFunc<QueryResponse<QueryType, QueryResult>> filter,
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

  void Initialize(size_t query_count) { query_count_ = query_count; }

  QueryResponseType Query(QueryType root, client::CancellationContext context) {
    responses_.emplace_back(root);
    return query_(root, context);
  }

  void CompleteQuery(QueryResponseType response) {
    std::lock_guard<std::mutex> lock(mutex_);

    accumulated_statistics_ += GetNetworkStatistics(response);

    if (response.IsSuccessful()) {
      responses_.back().list_of_tiles = response.GetResult();
      responses_.back().response = EmptyResponse(client::ApiNoResult());
    } else {
      const auto& error = response.GetError();
      if (error.GetErrorCode() == client::ErrorCode::Cancelled) {
        canceled_ = true;
      } else {
        // Collect all errors.
        responses_.back().response = {error};
      }
    }

    if (!--query_count_) {
      // process responses
      if (!IsErrorOccurs()) {
        ProcesQueryResponses();
      }
    }
  }

  bool IsErrorOccurs() {
    if (canceled_) {
      download_job_->OnPrefetchCompleted(
          {{client::ErrorCode::Cancelled, "Cancelled"}});
      return true;
    }
    for (const auto& resp : responses_) {
      if (resp.response.IsSuccessful()) {
        return false;
      }
    }
    // all queries fail, finish prefetch with error
    download_job_->OnPrefetchCompleted(
        {responses_.front().response.GetError()});
    return true;
  }

  void ProcesQueryResponses() {
    for (auto& resp : responses_) {
      if (filter_) {
        resp = filter_(resp);
      }
      query_result_size_ += resp.list_of_tiles.size();
    }

    if (!query_result_size_) {
      download_job_->OnPrefetchCompleted(PrefetchResult());
      return;
    }

    OLP_SDK_LOG_DEBUG_F("QueryMetadataJob", "Starting download, requests=%zu",
                        query_result_size_);

    download_job_->Initialize(query_result_size_, accumulated_statistics_);

    auto download_job = download_job_;

    bool all_download_tasks_triggered = true;

    auto trigger_download_tasks = [&]() {
      auto create_token = [&](const QueryResultItem& item,
                              const EmptyResponse& query_response) {
        const std::string& data_handle = item.second;
        const auto& item_key = item.first;

        auto result = task_sink_.AddTaskChecked(
            [=](client::CancellationContext context) {
              if (query_response.IsSuccessful()) {
                return download_job->Download(data_handle, context);
              } else {
                return download_job->PropagateError(query_response.GetError());
              }
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
      };

      VectorOfTokens tokens;

      for (const auto& resp : responses_) {
        const auto& items = resp.list_of_tiles;
        for (const auto& item : items) {
          tokens.emplace_back(create_token(item, resp.response));
        }
      }

      return CreateToken(std::move(tokens));
    };

    execution_context_.ExecuteOrCancelled(trigger_download_tasks);

    if (!all_download_tasks_triggered) {
      execution_context_.CancelOperation();
      download_job_->OnPrefetchCompleted(
          {{client::ErrorCode::Cancelled, "Cancelled"}});
    }
  }

 protected:
  std::vector<QueryResponse<QueryType, QueryResult>> responses_;
  QueryItemsFunc<ItemType, QueryType, QueryResponseType> query_;
  FilterItemsFunc<QueryResponse<QueryType, QueryResult>> filter_;
  size_t query_count_{0};
  bool canceled_{false};
  size_t query_result_size_{0};

  client::NetworkStatistics accumulated_statistics_;
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
