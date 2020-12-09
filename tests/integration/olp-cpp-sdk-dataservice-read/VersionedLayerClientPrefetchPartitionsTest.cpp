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

#include <algorithm>
#include <chrono>
#include <memory>
#include <numeric>
#include <string>

#include <matchers/NetworkUrlMatchers.h>
#include <mocks/NetworkMock.h>
#include <olp/authentication/Settings.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
#include <olp/dataservice/read/model/Partitions.h>
#include "HttpResponses.h"
#include "PlatformUrlsGenerator.h"
#include "ReadDefaultResponses.h"
#include "ResponseGenerator.h"
#include "VersionedLayerTestBase.h"

namespace {

namespace read = olp::dataservice::read;
namespace client = olp::client;
namespace http = olp::http;
using mockserver::ReadDefaultResponses;
using olp::http::HttpStatusCode;
using testing::_;
using VersionedLayerClientPrefetchPartitionsTest = VersionedLayerTestBase;

constexpr auto kTimeout = std::chrono::seconds(10);
constexpr auto kLayer = "testlayer";

std::vector<std::string> GeneratePartitionIds(size_t partitions_count) {
  std::vector<std::string> partitions;
  partitions.reserve(partitions_count);
  for (auto i = 0u; i < partitions_count; i++) {
    partitions.emplace_back(std::to_string(i));
  }
  return partitions;
}

TEST_F(VersionedLayerClientPrefetchPartitionsTest, PrefetchPartitions) {
  auto partitions_count = 3u;
  auto client =
      read::VersionedLayerClient(kCatalogHrn, kLayer, boost::none, settings_);
  auto partitions = GeneratePartitionIds(partitions_count);
  auto partitions_response =
      ReadDefaultResponses::GeneratePartitionsResponse(partitions_count);
  const auto request =
      read::PrefetchPartitionsRequest().WithPartitionIds(partitions);
  std::vector<std::string> partition_data_handles;
  {
    SCOPED_TRACE("Prefetch online");

    ExpectVersionRequest();
    ExpectQueryPartitionsRequest(partitions, partitions_response);
    for (const auto& partition : partitions_response.GetPartitions()) {
      ExpectBlobRequest(partition.GetDataHandle(), "data");
      partition_data_handles.push_back(partition.GetDataHandle());
    }

    auto future = client.PrefetchPartitions(request).GetFuture();
    ASSERT_NE(future.wait_for(std::chrono::seconds(kTimeout)),
              std::future_status::timeout);

    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful());
    const auto result = response.MoveResult();
    ASSERT_EQ(result.GetPartitions().size(), partitions_count);

    for (const auto& partition : result.GetPartitions()) {
      ASSERT_TRUE(client.IsCached(partition));
    }
  }
  {
    SCOPED_TRACE("Do not prefetch cached partitions twice");
    std::promise<read::PrefetchPartitionsResponse> promise;
    auto future = promise.get_future();
    auto token = client.PrefetchPartitions(
        request,
        [&promise](read::PrefetchPartitionsResponse response) {
          promise.set_value(std::move(response));
        },
        nullptr);
    ASSERT_NE(future.wait_for(std::chrono::seconds(kTimeout)),
              std::future_status::timeout);

    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful());
  }
  {
    SCOPED_TRACE("Get prefetched data from cache");
    std::promise<read::PrefetchPartitionsResponse> promise;

    auto future =
        client
            .GetData(read::DataRequest()
                         .WithFetchOption(read::FetchOptions::CacheOnly)
                         .WithDataHandle(partition_data_handles.front()))
            .GetFuture();
    ASSERT_NE(future.wait_for(std::chrono::seconds(kTimeout)),
              std::future_status::timeout);

    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful());
    ASSERT_FALSE(response.GetResult()->empty());
    std::string data_string(response.GetResult()->begin(),
                            response.GetResult()->end());
    ASSERT_EQ("data", data_string);
  }
  {
    SCOPED_TRACE("Get prefetched partition from cache");
    std::promise<read::PrefetchPartitionsResponse> promise;

    auto future =
        client
            .GetData(read::DataRequest()
                         .WithFetchOption(read::FetchOptions::CacheOnly)
                         .WithPartitionId(partitions.at(1)))
            .GetFuture();
    ASSERT_NE(future.wait_for(std::chrono::seconds(kTimeout)),
              std::future_status::timeout);

    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful());
    ASSERT_FALSE(response.GetResult()->empty());
    std::string data_string(response.GetResult()->begin(),
                            response.GetResult()->end());
    ASSERT_EQ("data", data_string);
  }
}

TEST_F(VersionedLayerClientPrefetchPartitionsTest, PrefetchPartitionsFails) {
  auto partitions_count = 3u;
  auto client =
      read::VersionedLayerClient(kCatalogHrn, kLayer, boost::none, settings_);
  auto partitions = GeneratePartitionIds(partitions_count);
  auto partitions_response =
      ReadDefaultResponses::GeneratePartitionsResponse(partitions_count);
  const auto request =
      read::PrefetchPartitionsRequest().WithPartitionIds(partitions);
  {
    SCOPED_TRACE("Get version fails");

    ExpectVersionRequest(
        http::NetworkResponse().WithStatus(HttpStatusCode::BAD_REQUEST));

    auto future = client.PrefetchPartitions(request).GetFuture();
    ASSERT_NE(future.wait_for(std::chrono::seconds(kTimeout)),
              std::future_status::timeout);

    auto response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
  }
  {
    SCOPED_TRACE("Query partitions fails");
    ExpectVersionRequest();
    ExpectQueryPartitionsRequest(
        partitions, partitions_response,
        http::NetworkResponse().WithStatus(HttpStatusCode::BAD_REQUEST));

    auto future = client.PrefetchPartitions(request).GetFuture();
    ASSERT_NE(future.wait_for(std::chrono::seconds(kTimeout)),
              std::future_status::timeout);

    auto response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
  }
  {
    SCOPED_TRACE("Get data fails");
    ExpectQueryPartitionsRequest(partitions, partitions_response);
    for (const auto& partition : partitions_response.GetPartitions()) {
      ExpectBlobRequest(
          partition.GetDataHandle(), "data",
          http::NetworkResponse().WithStatus(HttpStatusCode::BAD_REQUEST));
    }

    auto future = client.PrefetchPartitions(request).GetFuture();
    ASSERT_NE(future.wait_for(std::chrono::seconds(kTimeout)),
              std::future_status::timeout);

    auto response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
  }
}

TEST_F(VersionedLayerClientPrefetchPartitionsTest, PrefetchBatchFails) {
  // Should result in two metadata query
  constexpr auto partitions_count = 200u;
  constexpr auto size1 = 100;
  constexpr auto size2 = 100;

  auto client =
      read::VersionedLayerClient(kCatalogHrn, kLayer, version_, settings_);

  auto partitions = GeneratePartitionIds(partitions_count);
  auto partitions_response1 =
      ReadDefaultResponses::GeneratePartitionsResponse(size1);
  auto partitions_response2 =
      ReadDefaultResponses::GeneratePartitionsResponse(size2, size1);
  const auto request =
      read::PrefetchPartitionsRequest().WithPartitionIds(partitions);

  auto mock_partitions_response =
      [&](const read::model::Partitions& partitions) {
        for (const auto& partition : partitions.GetPartitions()) {
          ExpectBlobRequest(
              partition.GetDataHandle(), "data",
              http::NetworkResponse().WithStatus(HttpStatusCode::OK));
        }
      };

  {
    SCOPED_TRACE("All batch fails");

    ExpectQueryPartitionsRequest(
        {partitions.begin(), partitions.begin() + size1}, partitions_response1,
        http::NetworkResponse().WithStatus(HttpStatusCode::BAD_REQUEST));

    ExpectQueryPartitionsRequest(
        {partitions.begin() + size1, partitions.end()}, partitions_response2,
        http::NetworkResponse().WithStatus(HttpStatusCode::BAD_REQUEST));

    auto future = client.PrefetchPartitions(request).GetFuture();
    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);

    auto response = future.get();
    ASSERT_FALSE(response.IsSuccessful());

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }

  {
    SCOPED_TRACE("One batch fails");

    ExpectQueryPartitionsRequest(
        {partitions.begin(), partitions.begin() + size1}, partitions_response1,
        http::NetworkResponse().WithStatus(HttpStatusCode::BAD_REQUEST));

    ExpectQueryPartitionsRequest({partitions.begin() + size1, partitions.end()},
                                 partitions_response2);

    mock_partitions_response(partitions_response2);

    auto future = client.PrefetchPartitions(request).GetFuture();
    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);

    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful());

    const auto result = response.MoveResult();
    ASSERT_EQ(result.GetPartitions().size(), size1);

    for (const auto& partition : result.GetPartitions()) {
      ASSERT_TRUE(client.IsCached(partition));
    }

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(VersionedLayerClientPrefetchPartitionsTest, PrefetchPartitionsCancel) {
  auto partitions_count = 1u;
  auto client =
      read::VersionedLayerClient(kCatalogHrn, kLayer, boost::none, settings_);
  auto partitions = GeneratePartitionIds(partitions_count);
  auto partitions_response =
      ReadDefaultResponses::GeneratePartitionsResponse(partitions_count);
  const auto request =
      read::PrefetchPartitionsRequest().WithPartitionIds(partitions);

  std::promise<void> block_promise;
  auto block_future = block_promise.get_future();
  settings_.task_scheduler->ScheduleTask(
      [&block_future]() { block_future.get(); });
  auto cancellable = client.PrefetchPartitions(request);

  // cancel the request and unblock queue
  cancellable.GetCancellationToken().Cancel();
  block_promise.set_value();
  auto future = cancellable.GetFuture();

  ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

  auto data_response = future.get();

  EXPECT_FALSE(data_response.IsSuccessful());
  EXPECT_EQ(data_response.GetError().GetErrorCode(),
            client::ErrorCode::Cancelled);
}

TEST_F(VersionedLayerClientPrefetchPartitionsTest, CheckPriority) {
  auto priority = 300u;
  // this priority should be less than priority, but greater than LOW
  auto finish_task_priority = 200u;
  auto partitions_count = 3u;
  auto client =
      read::VersionedLayerClient(kCatalogHrn, kLayer, boost::none, settings_);
  auto partitions = GeneratePartitionIds(partitions_count);
  auto partitions_response =
      ReadDefaultResponses::GeneratePartitionsResponse(partitions_count);
  const auto request = read::PrefetchPartitionsRequest()
                           .WithPartitionIds(partitions)
                           .WithPriority(priority);

  ExpectVersionRequest();
  ExpectQueryPartitionsRequest(partitions, partitions_response);
  for (const auto& partition : partitions_response.GetPartitions()) {
    ExpectBlobRequest(partition.GetDataHandle(), "data");
  }

  auto scheduler = settings_.task_scheduler;
  std::promise<void> block_promise;
  std::promise<void> finish_promise;
  auto block_future = block_promise.get_future();
  auto finish_future = finish_promise.get_future();

  // block task scheduler
  scheduler->ScheduleTask([&]() { block_future.wait_for(kTimeout); },
                          std::numeric_limits<uint32_t>::max());

  auto future = client.PrefetchPartitions(request).GetFuture();
  scheduler->ScheduleTask(
      [&]() {
        EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)),
                  std::future_status::ready);
        finish_promise.set_value();
      },
      finish_task_priority);

  // unblock queue
  block_promise.set_value();

  ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
  ASSERT_NE(finish_future.wait_for(kTimeout), std::future_status::timeout);

  auto response = future.get();
  ASSERT_TRUE(response.IsSuccessful());
  const auto result = response.MoveResult();
  ASSERT_EQ(result.GetPartitions().size(), partitions_count);

  for (const auto& partition : result.GetPartitions()) {
    ASSERT_TRUE(client.IsCached(partition));
  }
}

TEST_F(VersionedLayerClientPrefetchPartitionsTest, PrefetchProgress) {
  auto partitions_count = 201u;  // 3 query
  auto client =
      read::VersionedLayerClient(kCatalogHrn, kLayer, version_, settings_);
  auto partitions = GeneratePartitionIds(partitions_count);

  auto size1 = 100;
  auto size2 = 100;
  auto size3 = 1;

  auto partitions_response1 =
      ReadDefaultResponses::GeneratePartitionsResponse(size1);
  auto partitions_response2 =
      ReadDefaultResponses::GeneratePartitionsResponse(size2, size1);
  auto partitions_response3 =
      ReadDefaultResponses::GeneratePartitionsResponse(size3, size1 + size2);
  const auto request =
      read::PrefetchPartitionsRequest().WithPartitionIds(partitions);

  ExpectQueryPartitionsRequest(
      std::vector<std::string>(partitions.begin(), partitions.begin() + size1),
      partitions_response1);
  ExpectQueryPartitionsRequest(
      std::vector<std::string>(partitions.begin() + size1,
                               partitions.begin() + size1 + size2),
      partitions_response2);
  ExpectQueryPartitionsRequest(
      std::vector<std::string>(partitions.begin() + size1 + size2,
                               partitions.begin() + size1 + size2 + size3),
      partitions_response3);

  auto mock_partitions_response =
      [&](const read::model::Partitions& partitions) {
        for (const auto& partition : partitions.GetPartitions()) {
          ExpectBlobRequest(partition.GetDataHandle(), "data",
                            http::NetworkResponse()
                                .WithStatus(HttpStatusCode::OK)
                                .WithBytesDownloaded(4)
                                .WithBytesUploaded(1));
        }
      };
  mock_partitions_response(partitions_response1);
  mock_partitions_response(partitions_response2);
  mock_partitions_response(partitions_response3);

  struct Status {
    MOCK_METHOD(void, Op, (read::PrefetchPartitionsStatus));
  };

  Status status_object;
  size_t bytes_transferred{0};
  {
    auto matches = [](uint32_t prefetched, uint32_t total) {
      return [=](read::PrefetchPartitionsStatus status) -> bool {
        return status.prefetched_partitions == prefetched &&
               status.total_partitions_to_prefetch == total;
      };
    };
    for (auto i = 0u; i < partitions_count; i++) {
      EXPECT_CALL(status_object,
                  Op(testing::Truly(matches(i + 1, partitions_count))))
          .Times(1);
    }
  }

  auto future =
      client
          .PrefetchPartitions(request,
                              [&](read::PrefetchPartitionsStatus status) {
                                status_object.Op(status);
                                bytes_transferred = status.bytes_transferred;
                              })
          .GetFuture();
  ASSERT_NE(future.wait_for(std::chrono::seconds(kTimeout)),
            std::future_status::timeout);

  auto response = future.get();
  ASSERT_TRUE(response.IsSuccessful());
  const auto result = response.MoveResult();
  ASSERT_EQ(result.GetPartitions().size(), partitions_count);

  for (const auto& partition : result.GetPartitions()) {
    ASSERT_TRUE(client.IsCached(partition));
  }
  EXPECT_GE(bytes_transferred, partitions_count * 5);
  testing::Mock::VerifyAndClearExpectations(&status_object);
}

}  // namespace
