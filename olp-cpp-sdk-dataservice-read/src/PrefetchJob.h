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
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/dataservice/read/Types.h>

#include <olp/core/client/ExtendedApiResponse.h>

namespace olp {
namespace dataservice {
namespace read {

using Roots = std::vector<geo::TileKey>;
using QueryResult = std::map<geo::TileKey, std::string>;

using QueryResponse = client::ExtendedApiResponse<QueryResult, client::ApiError,
                                                  client::NetworkStatistics>;
using ExtendedDataResponse =
    client::ExtendedApiResponse<model::Data, client::ApiError,
                                client::NetworkStatistics>;

// Prototype of function used to download quad tree.
using QueryFunc =
    std::function<QueryResponse(geo::TileKey, client::CancellationContext)>;
// Prototype of function used to filter the tiles.
using FilterFunc = std::function<QueryResult(QueryResult)>;
// Prototype of function used to download a tile.
using DownloadFunc = std::function<ExtendedDataResponse(
    std::string, client::CancellationContext)>;

class DownloadTilesJob {
 public:
  DownloadTilesJob(DownloadFunc download,
                   PrefetchTilesResponseCallback user_callback,
                   PrefetchStatusCallback status_callback);

  void Initialize(size_t tiles_count, client::NetworkStatistics statistics);

  ExtendedDataResponse Download(const std::string& data_handle,
                                client::CancellationContext context);

  void CompleteTile(geo::TileKey tile, ExtendedDataResponse response);

  void OnPrefetchCompleted(PrefetchTilesResponse result);

 private:
  DownloadFunc download_;
  PrefetchTilesResponseCallback user_callback_;
  PrefetchStatusCallback status_callback_;

  size_t download_task_count_{0};
  size_t total_download_task_count_{0};

  size_t requests_succeeded_{0};
  size_t requests_failed_{0};

  client::NetworkStatistics accumulated_statistics_;

  PrefetchTilesResult prefetch_result_;

  std::mutex mutex_;
};

class QueryMetadataJob {
 public:
  QueryMetadataJob(QueryFunc query, FilterFunc filter,
                   std::shared_ptr<DownloadTilesJob> download_job,
                   std::shared_ptr<thread::TaskScheduler> task_scheduler,
                   std::shared_ptr<client::PendingRequests> pending_requests,
                   client::CancellationContext execution_context);

  void Initialize(size_t query_count);

  QueryResponse Query(geo::TileKey root, client::CancellationContext context);

  void CompleteQuery(QueryResponse response);

 private:
  QueryFunc query_;
  FilterFunc filter_;
  size_t query_count_{0};

  bool canceled_{false};

  QueryResult query_result_;

  client::NetworkStatistics accumulated_statistics_;

  boost::optional<client::ApiError> query_error_;

  std::shared_ptr<DownloadTilesJob> download_job_;
  std::shared_ptr<thread::TaskScheduler> task_scheduler_;
  std::shared_ptr<client::PendingRequests> pending_requests_;
  client::CancellationContext execution_context_;

  std::mutex mutex_;
};

class PrefetchHelper {
 public:
  static client::CancellationToken Prefetch(
      const Roots& roots, QueryFunc query, FilterFunc filter,
      DownloadFunc download, PrefetchTilesResponseCallback user_callback,
      PrefetchStatusCallback status_callback,
      std::shared_ptr<thread::TaskScheduler> task_scheduler,
      std::shared_ptr<client::PendingRequests> pending_requests);
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
