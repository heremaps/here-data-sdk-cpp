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

#include "PrefetchJob.h"

#include <olp/core/logging/Log.h>
#include <olp/dataservice/read/PrefetchTileResult.h>

#include "Common.h"
#include "ExtendedApiResponseHelpers.h"

namespace {
constexpr auto kLogTag = "PrefetchJob";

using VectorOfTokens = std::vector<olp::client::CancellationToken>;

size_t GetAccumulatedBytes(const olp::client::NetworkStatistics& statistics) {
  // This narrow cast is necessary to avoid narrowing compiler errors like
  // -Wc++11-narrowing when building for 32bit targets.
  const auto bytes_transferred =
      statistics.GetBytesDownloaded() + statistics.GetBytesUploaded();
  return static_cast<size_t>(bytes_transferred &
                             std::numeric_limits<size_t>::max());
}

olp::client::CancellationToken CreateToken(VectorOfTokens tokens) {
  return olp::client::CancellationToken(std::bind(
      [](const VectorOfTokens& tokens) {
        std::for_each(std::begin(tokens), std::end(tokens),
                      [](const olp::client::CancellationToken& token) {
                        token.Cancel();
                      });
      },
      std::move(tokens)));
}

}  // namespace

namespace olp {
namespace dataservice {
namespace read {

DownloadTilesJob::DownloadTilesJob(DownloadFunc download,
                                   PrefetchTilesResponseCallback user_callback,
                                   PrefetchStatusCallback status_callback)
    : download_(std::move(download)),
      user_callback_(std::move(user_callback)),
      status_callback_(std::move(status_callback)) {}

void DownloadTilesJob::Initialize(size_t tiles_count,
                                  client::NetworkStatistics statistics) {
  download_task_count_ = total_download_task_count_ = tiles_count;
  accumulated_statistics_ = statistics;
}

ExtendedDataResponse DownloadTilesJob::Download(
    const std::string& data_handle, client::CancellationContext context) {
  return download_(data_handle, context);
}

void DownloadTilesJob::CompleteTile(geo::TileKey tile,
                                    ExtendedDataResponse response) {
  std::lock_guard<std::mutex> lock(mutex_);
  accumulated_statistics_ += GetNetworkStatistics(response);

  if (response.IsSuccessful()) {
    prefetch_result_.push_back(
        std::make_shared<PrefetchTileResult>(tile, PrefetchTileNoError()));
    requests_succeeded_++;
  } else {
    prefetch_result_.push_back(
        std::make_shared<PrefetchTileResult>(tile, response.GetError()));
    requests_failed_++;
  }

  if (status_callback_) {
    status_callback_(
        PrefetchStatus{prefetch_result_.size(), total_download_task_count_,
                       GetAccumulatedBytes(accumulated_statistics_)});
  }

  if (!--download_task_count_) {
    OLP_SDK_LOG_DEBUG_F(kLogTag, "Download complete, succeeded=%zu, failed=%zu",
                        requests_succeeded_, requests_failed_);

    OnPrefetchCompleted(std::move(prefetch_result_));
  }
}

void DownloadTilesJob::OnPrefetchCompleted(PrefetchTilesResponse result) {
  auto prefetch_callback = std::move(user_callback_);
  prefetch_callback(std::move(result));
}

///////////////////////////////////////////////////////////////////////////////////

QueryMetadataJob::QueryMetadataJob(
    QueryFunc query, FilterFunc filter,
    std::shared_ptr<DownloadTilesJob> download_job,
    std::shared_ptr<thread::TaskScheduler> task_scheduler,
    std::shared_ptr<client::PendingRequests> pending_requests,
    client::CancellationContext execution_context)
    : query_(std::move(query)),
      filter_(std::move(filter)),
      download_job_(std::move(download_job)),
      task_scheduler_(std::move(task_scheduler)),
      pending_requests_(std::move(pending_requests)),
      execution_context_(execution_context) {}

void QueryMetadataJob::Initialize(size_t query_count) {
  query_count_ = query_count;
}

QueryResponse QueryMetadataJob::Query(geo::TileKey root,
                                      client::CancellationContext context) {
  return query_(root, context);
}

void QueryMetadataJob::CompleteQuery(QueryResponse response) {
  std::lock_guard<std::mutex> lock(mutex_);

  accumulated_statistics_ += GetNetworkStatistics(response);

  if (response.IsSuccessful()) {
    auto tiles = response.MoveResult();
    query_result_.insert(std::make_move_iterator(tiles.begin()),
                         std::make_move_iterator(tiles.end()));
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
      download_job_->OnPrefetchCompleted(PrefetchTilesResponse::ResultType());
      return;
    }

    OLP_SDK_LOG_DEBUG_F(kLogTag, "Starting download, requests=%zu",
                        query_result_.size());

    download_job_->Initialize(query_result_.size(), accumulated_statistics_);

    auto download_job = download_job_;

    execution_context_.ExecuteOrCancelled([&]() {
      VectorOfTokens tokens;
      std::transform(
          std::begin(query_result_), std::end(query_result_),
          std::back_inserter(tokens), [&](const QueryResult::value_type& tile) {
            const std::string& data_handle = tile.second;
            const geo::TileKey& tile_key = tile.first;
            return AddTask(
                task_scheduler_, pending_requests_,
                [=](client::CancellationContext context) {
                  return download_job->Download(data_handle, context);
                },
                [=](ExtendedDataResponse response) {
                  download_job->CompleteTile(tile_key, std::move(response));
                });
          });
      return CreateToken(std::move(tokens));
    });
  }
}

client::CancellationToken PrefetchHelper::Prefetch(
    const Roots& roots, QueryFunc query, FilterFunc filter,
    DownloadFunc download, PrefetchTilesResponseCallback user_callback,
    PrefetchStatusCallback status_callback,
    std::shared_ptr<thread::TaskScheduler> task_scheduler,
    std::shared_ptr<client::PendingRequests> pending_requests) {
  client::CancellationContext execution_context;

  auto download_job = std::make_shared<DownloadTilesJob>(
      std::move(download), std::move(user_callback),
      std::move(status_callback));

  auto query_job = std::make_shared<QueryMetadataJob>(
      std::move(query), std::move(filter), download_job, task_scheduler,
      pending_requests, execution_context);

  query_job->Initialize(roots.size());

  OLP_SDK_LOG_DEBUG_F(kLogTag, "Starting queries, requests=%zu", roots.size());

  execution_context.ExecuteOrCancelled([&]() {
    VectorOfTokens tokens;
    std::transform(std::begin(roots), std::end(roots),
                   std::back_inserter(tokens), [&](geo::TileKey root) {
                     return AddTask(
                         task_scheduler, pending_requests,
                         [=](client::CancellationContext context) {
                           return query_job->Query(root, context);
                         },
                         [=](QueryResponse response) {
                           query_job->CompleteQuery(std::move(response));
                         });
                   });
    return CreateToken(std::move(tokens));
  });

  return client::CancellationToken(
      [execution_context]() mutable { execution_context.CancelOperation(); });
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
