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

#include <gtest/gtest.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/NetworkSettings.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/dataservice/read/FetchOptions.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
// clang-format off
#include "generated/serializer/PartitionsSerializer.h"
#include "generated/serializer/JsonSerializer.h"
// clang-format on
#include <string>
#include "ApiDefaultResponses.h"
#include "MockServerHelper.h"
#include "ReadDefaultResponses.h"
#include "SetupMockServer.h"
#include "Utils.h"

namespace {

namespace dr = olp::dataservice::read;

constexpr auto kWaitTimeout = std::chrono::seconds(10);

class VersionedLayerClientPrefetchPartitionsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();
    settings_ = mockserver::SetupMockServer::CreateSettings(network);
    mock_server_client_ =
        mockserver::SetupMockServer::CreateMockServer(network, kTestHrn);
  }

  void TearDown() override {
    auto network = std::move(settings_->network_request_handler);
    settings_.reset();
    mock_server_client_.reset();
  }

  std::string GetPartitions() {
    return "/query/v1/catalogs/" + kTestHrn + "/layers/" + kLayer +
           "/partitions";
  }

  std::shared_ptr<olp::client::OlpClientSettings> settings_;
  std::shared_ptr<mockserver::MockServerHelper> mock_server_client_;
  const std::string kTestHrn =
      "hrn:here:data::olp-here-test:hereos-internal-test";
  const std::string kLayer = "testlayer";
  const uint32_t kVersion = 44;
};

TEST_F(VersionedLayerClientPrefetchPartitionsTest, PrefetchPartitions) {
  olp::client::HRN hrn(kTestHrn);

  const auto data = mockserver::ReadDefaultResponses::GenerateData();

  auto client = dr::VersionedLayerClient(hrn, kLayer, boost::none, *settings_);

  {
    SCOPED_TRACE("Prefetch partitions");
    std::vector<std::string> partitions;
    partitions.reserve(10);
    for (auto i = 0u; i < 10; i++) {
      partitions.emplace_back(std::to_string(i));
    }
    const auto request =
        dr::PrefetchPartitionsRequest().WithPartitionIds(partitions);
    {
      mock_server_client_->MockAuth();
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
              kTestHrn));
      mock_server_client_->MockGetVersionResponse(
          mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));

      mock_server_client_->MockGetResponse(
          mockserver::ReadDefaultResponses::GeneratePartitionsResponse(10),
          GetPartitions());
      for (auto i = 0u; i < 10; i++) {
        mock_server_client_->MockGetResponse(
            kLayer,
            mockserver::ReadDefaultResponses::GenerateDataHandle(partitions[i]),
            data);
      }
    }
    std::promise<dr::PrefetchPartitionsResponse> promise;
    auto future = promise.get_future();
    auto token = client.PrefetchPartitions(
        request, [&promise](dr::PrefetchPartitionsResponse response) {
          promise.set_value(std::move(response));
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);

    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    const auto result = response.MoveResult();

    EXPECT_EQ(result.GetPartitions().size(), 10u);
    for (const auto& partition : partitions) {
      ASSERT_TRUE(client.IsCached(partition));

      auto data_response =
          client
              .GetData(
                  dr::DataRequest().WithPartitionId(partition).WithFetchOption(
                      dr::FetchOptions::CacheOnly))
              .GetFuture()
              .get();
      ASSERT_TRUE(data_response.IsSuccessful());
      EXPECT_EQ(data_response.GetResult()->size(), 64u);
    }
    EXPECT_TRUE(mock_server_client_->Verify());
  }
  {
    SCOPED_TRACE("Call get data than prefetch");
    auto partition = "12";
    const auto data = mockserver::ReadDefaultResponses::GenerateData();
    const auto request =
        dr::PrefetchPartitionsRequest().WithPartitionIds({partition});
    {
      mock_server_client_->MockGetResponse(
          mockserver::ReadDefaultResponses::GeneratePartitionsResponse(1, 12),
          GetPartitions());
      mock_server_client_->MockGetResponse(
          kLayer,
          mockserver::ReadDefaultResponses::GenerateDataHandle(partition),
          data);
    }
    auto data_future =
        client.GetData(dr::DataRequest().WithPartitionId(partition))
            .GetFuture();
    ASSERT_NE(data_future.wait_for(kWaitTimeout), std::future_status::timeout);

    auto data_response = data_future.get();
    ASSERT_TRUE(data_response.IsSuccessful());
    EXPECT_EQ(data_response.GetResult()->size(), data.size());

    auto future = client.PrefetchPartitions(request).GetFuture();
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);

    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << response.GetError().GetMessage().c_str();
    const auto result = response.MoveResult();

    EXPECT_EQ(result.GetPartitions().size(), 1u);
    ASSERT_TRUE(client.IsCached(partition));

    EXPECT_TRUE(mock_server_client_->Verify());
  }
}

TEST_F(VersionedLayerClientPrefetchPartitionsTest, PrefetchPartitionFail) {
  olp::client::HRN hrn(kTestHrn);

  auto client = dr::VersionedLayerClient(hrn, kLayer, boost::none, *settings_);
  {
    SCOPED_TRACE("Empty request");
    const auto request = dr::PrefetchPartitionsRequest();
    {
      mock_server_client_->MockAuth();
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
              kTestHrn));
    }
    std::promise<dr::PrefetchPartitionsResponse> promise;
    auto future = promise.get_future();
    auto token = client.PrefetchPartitions(
        request, [&promise](dr::PrefetchPartitionsResponse response) {
          promise.set_value(std::move(response));
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);

    auto response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
              response.GetError().GetErrorCode());
  }
  {
    SCOPED_TRACE("Failed request");
    const auto request =
        dr::PrefetchPartitionsRequest().WithPartitionIds({"1"});
    {
      mock_server_client_->MockGetVersionResponse(
          mockserver::ReadDefaultResponses::GenerateVersionResponse(kVersion));

      mock_server_client_->MockGetError(
          {olp::http::HttpStatusCode::NOT_FOUND, "Not found"}, GetPartitions());
    }
    std::promise<dr::PrefetchPartitionsResponse> promise;
    auto future = promise.get_future();
    auto token = client.PrefetchPartitions(
        request, [&promise](dr::PrefetchPartitionsResponse response) {
          promise.set_value(std::move(response));
        });
    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);

    auto response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(olp::client::ErrorCode::NotFound,
              response.GetError().GetErrorCode());
  }
}

}  // namespace
