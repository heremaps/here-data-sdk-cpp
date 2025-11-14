/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include <chrono>
#include <future>
#include <thread>

#include <matchers/NetworkUrlMatchers.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/client/ApiLookupClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/utils/Url.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/TileRequest.h>
#include <repositories/DataRepository.h>
#include <repositories/NamedMutex.h>
#include <repositories/PartitionsCacheRepository.h>

constexpr auto kUrlLookup =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data::olp-here-test:hereos-internal-test-v2/apis)";

constexpr auto kUrlBlobData269 =
    R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/4eed6ed1-0d32-43b9-ae79-043cb4256432)";

constexpr auto kUrlBlobData5904591 =
    R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1)";

constexpr auto kUrlBlobData1476147 =
    R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/95c5c703-e00e-4c38-841e-e419367474f1)";

constexpr auto kUrlResponseLookup =
    R"jsonString([{"api":"query","version":"v1","baseURL":"https://sab.query.data.api.platform.here.com/query/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2","parameters":{}},{"api":"blob","version":"v1","baseURL":"https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";

constexpr auto kUrlResponse403 =
    R"jsonString("Forbidden - A catalog with the specified HRN doesn't exist or access to this catalog is forbidden)jsonString";

constexpr auto kUrlBlobDataHandle = R"(4eed6ed1-0d32-43b9-ae79-043cb4256432)";

const auto kUrlQueryTreeIndex =
    R"(https://sab.query.data.api.platform.here.com/query/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/versions/4/quadkeys/23064/depths/4?additionalFields=)" +
    olp::utils::Url::Encode(R"(checksum,crc,dataSize,compressedDataSize)");

constexpr auto kSubQuads =
    R"jsonString(
      {
        "subQuads": [
          {"subQuadKey":"115","version":4,"dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1","checksum":"xxx","compressedDataSize":10,"dataSize":15,"crc":"aaa"},
          {"subQuadKey":"463","version":4,"dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1","checksum":"xxx","compressedDataSize":10,"dataSize":15,"crc":"aaa"}],
        "parentQuads": []
      })jsonString";

namespace {

using olp::client::ApiLookupClient;
using olp::dataservice::read::repository::DataRepository;
using testing::_;

const std::string kLayerId = "testlayer";
const std::string kService = "blob";

class DataRepositoryTest : public ::testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;

  static std::string GetTestCatalog();

  std::shared_ptr<olp::client::OlpClientSettings> settings_;
  std::shared_ptr<NetworkMock> network_mock_;
  std::shared_ptr<olp::client::ApiLookupClient> lookup_client_;
  olp::client::HRN hrn_;
};

void DataRepositoryTest::SetUp() {
  network_mock_ = std::make_shared<NetworkMock>();
  settings_ = std::make_shared<olp::client::OlpClientSettings>();
  settings_->cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
  settings_->network_request_handler = network_mock_;

  hrn_ = olp::client::HRN(GetTestCatalog());
  lookup_client_ =
      std::make_shared<olp::client::ApiLookupClient>(hrn_, *settings_);
}

void DataRepositoryTest::TearDown() {
  settings_.reset();
  network_mock_.reset();
}

std::string DataRepositoryTest::GetTestCatalog() {
  return "hrn:here:data::olp-here-test:hereos-internal-test-v2";
}

TEST_F(DataRepositoryTest, GetBlobData) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUrlResponseLookup));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlBlobData269), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  olp::client::CancellationContext context;

  olp::dataservice::read::model::Partition partition;
  partition.SetDataHandle(kUrlBlobDataHandle);

  olp::client::HRN hrn(GetTestCatalog());
  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client);
  auto response = repository.GetBlobData(
      kLayerId, kService, partition,
      olp::dataservice::read::FetchOptions::OnlineIfNotFound,
      olp::porting::none, context, false);

  ASSERT_TRUE(response.IsSuccessful());
}

TEST_F(DataRepositoryTest, GetBlobDataApiLookupFailed403) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::FORBIDDEN),
                                   kUrlResponse403));

  olp::client::CancellationContext context;

  olp::dataservice::read::model::Partition partition;
  partition.SetDataHandle(kUrlBlobDataHandle);

  olp::client::HRN hrn(GetTestCatalog());
  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client);
  auto response = repository.GetBlobData(
      kLayerId, kService, partition,
      olp::dataservice::read::FetchOptions::OnlineIfNotFound,
      olp::porting::none, context, false);

  ASSERT_FALSE(response.IsSuccessful());
}

TEST_F(DataRepositoryTest, GetBlobDataNoDataHandle) {
  olp::client::CancellationContext context;
  olp::dataservice::read::DataRequest request;
  olp::client::HRN hrn(GetTestCatalog());
  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client);

  auto response = repository.GetBlobData(
      kLayerId, kService, olp::dataservice::read::model::Partition(),
      olp::dataservice::read::FetchOptions::OnlineIfNotFound,
      olp::porting::none, context, false);

  ASSERT_FALSE(response.IsSuccessful());
}

TEST_F(DataRepositoryTest, GetBlobDataFailedDataFetch403) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUrlResponseLookup));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlBlobData269), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::FORBIDDEN),
                                   kUrlResponse403));

  olp::client::CancellationContext context;

  olp::dataservice::read::model::Partition partition;
  partition.SetDataHandle(kUrlBlobDataHandle);

  olp::client::HRN hrn(GetTestCatalog());
  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client);
  auto response = repository.GetBlobData(
      kLayerId, kService, partition,
      olp::dataservice::read::FetchOptions::OnlineIfNotFound,
      olp::porting::none, context, false);

  ASSERT_FALSE(response.IsSuccessful());
}

TEST_F(DataRepositoryTest, GetBlobDataCache) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUrlResponseLookup));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlBlobData269), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  olp::client::CancellationContext context;

  olp::dataservice::read::model::Partition partition;
  partition.SetDataHandle(kUrlBlobDataHandle);

  olp::client::HRN hrn(GetTestCatalog());

  // This should download data from network and cache it
  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client);
  auto response = repository.GetBlobData(
      kLayerId, kService, partition,
      olp::dataservice::read::FetchOptions::OnlineIfNotFound,
      olp::porting::none, context, false);

  ASSERT_TRUE(response.IsSuccessful());

  // This call should not do any network calls and use already cached values
  // instead
  response = repository.GetBlobData(
      kLayerId, kService, partition,
      olp::dataservice::read::FetchOptions::OnlineIfNotFound,
      olp::porting::none, context, false);

  ASSERT_TRUE(response.IsSuccessful());
}

TEST_F(DataRepositoryTest, GetBlobDataImmediateCancel) {
  ON_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        kUrlResponseLookup));

  ON_CALL(*network_mock_, Send(IsGetRequest(kUrlBlobData269), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        "someData"));

  olp::client::CancellationContext context;

  olp::dataservice::read::model::Partition partition;
  partition.SetDataHandle(kUrlBlobDataHandle);

  olp::client::HRN hrn(GetTestCatalog());

  context.CancelOperation();
  ASSERT_TRUE(context.IsCancelled());

  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client);
  auto response = repository.GetBlobData(
      kLayerId, kService, partition,
      olp::dataservice::read::FetchOptions::OnlineIfNotFound,
      olp::porting::none, context, false);

  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataRepositoryTest, GetBlobDataInProgressCancel) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUrlResponseLookup));

  olp::client::CancellationContext context;

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlBlobData269), _, _, _, _))
      .WillOnce(
          [&](olp::http::NetworkRequest, olp::http::Network::Payload,
              olp::http::Network::Callback, olp::http::Network::HeaderCallback,
              olp::http::Network::DataCallback) -> olp::http::SendOutcome {
            std::thread([&]() { context.CancelOperation(); }).detach();
            constexpr auto unused_request_id = 12;
            return olp::http::SendOutcome(unused_request_id);
          });
  EXPECT_CALL(*network_mock_, Cancel(_)).WillOnce(testing::Return());

  olp::dataservice::read::model::Partition partition;
  partition.SetDataHandle(kUrlBlobDataHandle);

  olp::client::HRN hrn(GetTestCatalog());

  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client);
  auto response = repository.GetBlobData(
      kLayerId, kService, partition,
      olp::dataservice::read::FetchOptions::OnlineIfNotFound,
      olp::porting::none, context, false);

  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataRepositoryTest, GetBlobDataSimultaniousFailedCalls) {
  // The lookup data must be requested from the network only once
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUrlResponseLookup));

  std::promise<void> network_request_started_promise;
  std::promise<void> finish_network_request_promise;

  auto wait = [&]() {
    network_request_started_promise.set_value();
    finish_network_request_promise.get_future().wait();
  };

  // The blob data must be requested from the network only once
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlBlobData269), _, _, _, _))
      .WillOnce(testing::DoAll(
          testing::InvokeWithoutArgs(wait),
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::REQUEST_TIMEOUT),
                             "Timeout")));

  olp::client::CancellationContext context;
  olp::dataservice::read::repository::NamedMutexStorage storage;

  olp::dataservice::read::model::Partition partition;
  partition.SetDataHandle(kUrlBlobDataHandle);

  olp::client::HRN hrn(GetTestCatalog());
  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client, storage);

  // Start first request in a separate thread
  std::thread first_request_thread([&]() {
    auto response = repository.GetBlobData(
        kLayerId, kService, partition,
        olp::dataservice::read::FetchOptions::OnlineIfNotFound,
        olp::porting::none, context, false);
    EXPECT_FALSE(response.IsSuccessful());
  });

  // Wait untill network request processing started
  network_request_started_promise.get_future().wait();

  // Get a mutex from the storage. It guarantees that when the second thread
  // accuares the mutex, the stored error will not be cleaned up in scope of
  // ReleaseLock call from the first thread
  olp::dataservice::read::repository::NamedMutex mutex(
      storage, hrn.ToString() + kService + kUrlBlobDataHandle, context);

  // Start second request in a separate thread
  std::thread second_request_thread([&]() {
    auto response = repository.GetBlobData(
        kLayerId, kService, partition,
        olp::dataservice::read::FetchOptions::OnlineIfNotFound,
        olp::porting::none, context, false);
    EXPECT_FALSE(response.IsSuccessful());
  });

  finish_network_request_promise.set_value();
  first_request_thread.join();
  second_request_thread.join();
}

TEST_F(DataRepositoryTest, GetVersionedDataTile) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUrlResponseLookup));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kUrlQueryTreeIndex), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kSubQuads));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kUrlBlobData5904591), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  olp::client::HRN hrn(GetTestCatalog());
  int64_t version = 4;

  // request data for tile
  {
    auto request = olp::dataservice::read::TileRequest().WithTileKey(
        olp::geo::TileKey::FromHereTile("5904591"));
    olp::client::CancellationContext context;
    ApiLookupClient lookup_client(hrn, *settings_);
    DataRepository repository(hrn, *settings_, lookup_client);
    auto response =
        repository.GetVersionedTile(kLayerId, request, version, context);

    ASSERT_TRUE(response.IsSuccessful());
  }

  // second request for another tile key, data handle should be found in cache,
  // no need to query online
  {
    ON_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kUrlResponseLookup));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kUrlBlobData1476147), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     "someData"));

    auto request = olp::dataservice::read::TileRequest().WithTileKey(
        olp::geo::TileKey::FromHereTile("1476147"));
    olp::client::CancellationContext context;
    ApiLookupClient lookup_client(hrn, *settings_);
    DataRepository repository(hrn, *settings_, lookup_client);
    auto response =
        repository.GetVersionedTile(kLayerId, request, version, context);

    ASSERT_TRUE(response.IsSuccessful());
  }
}

TEST_F(DataRepositoryTest, GetVersionedDataTileOnlineOnly) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .Times(2)
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUrlResponseLookup))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUrlResponseLookup));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kUrlQueryTreeIndex), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kSubQuads));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kUrlBlobData5904591), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  olp::client::HRN hrn(GetTestCatalog());
  int64_t version = 4;

  // request for tile key, but use OnlineOnly option,
  {
    auto request =
        olp::dataservice::read::TileRequest()
            .WithTileKey(olp::geo::TileKey::FromHereTile("5904591"))
            .WithFetchOption(olp::dataservice::read::FetchOptions::OnlineOnly);
    olp::client::CancellationContext context;
    ApiLookupClient lookup_client(hrn, *settings_);
    DataRepository repository(hrn, *settings_, lookup_client);
    auto response =
        repository.GetVersionedTile(kLayerId, request, version, context);

    ASSERT_TRUE(response.IsSuccessful());
  }
}

TEST_F(DataRepositoryTest, GetVersionedDataTileImmediateCancel) {
  olp::client::HRN hrn(GetTestCatalog());
  int64_t version = 4;

  auto request = olp::dataservice::read::TileRequest().WithTileKey(
      olp::geo::TileKey::FromHereTile("5904591"));
  olp::client::CancellationContext context;

  context.CancelOperation();
  ASSERT_TRUE(context.IsCancelled());

  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client);
  auto response =
      repository.GetVersionedTile(kLayerId, request, version, context);

  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataRepositoryTest, GetVersionedDataTileInProgressCancel) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUrlResponseLookup));

  olp::client::CancellationContext context;

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kUrlQueryTreeIndex), _, _, _, _))
      .WillOnce(
          [&](olp::http::NetworkRequest, olp::http::Network::Payload,
              olp::http::Network::Callback, olp::http::Network::HeaderCallback,
              olp::http::Network::DataCallback) -> olp::http::SendOutcome {
            std::thread([&]() { context.CancelOperation(); }).detach();
            constexpr auto unused_request_id = 12;
            return olp::http::SendOutcome(unused_request_id);
          });

  EXPECT_CALL(*network_mock_, Cancel(_)).WillOnce(testing::Return());

  olp::client::HRN hrn(GetTestCatalog());
  int64_t version = 4;

  auto request = olp::dataservice::read::TileRequest().WithTileKey(
      olp::geo::TileKey::FromHereTile("5904591"));

  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client);
  auto response =
      repository.GetVersionedTile(kLayerId, request, version, context);

  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataRepositoryTest, GetVersionedDataTileReturnEmpty) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUrlResponseLookup));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kUrlQueryTreeIndex), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "no_data_handle_in_responce"));

  olp::client::HRN hrn(GetTestCatalog());
  int64_t version = 4;
  olp::client::CancellationContext context;

  auto request = olp::dataservice::read::TileRequest().WithTileKey(
      olp::geo::TileKey::FromHereTile("5904591"));

  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client);
  auto response =
      repository.GetVersionedTile(kLayerId, request, version, context);

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Unknown);
  ASSERT_EQ(response.GetError().GetMessage(),
            "Failed to parse quad tree response");
}

TEST_F(DataRepositoryTest, GetBlobDataCancelParralellRequest) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .WillRepeatedly(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::OK),
                             kUrlResponseLookup));

  constexpr auto wait_time = std::chrono::seconds(1);
  auto wait = [&wait_time]() { std::this_thread::sleep_for(wait_time); };

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlBlobData269), _, _, _, _))
      .WillRepeatedly(testing::DoAll(
          testing::InvokeWithoutArgs(wait),
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::OK),
                             "Some Data")));

  olp::client::CancellationContext context;
  olp::dataservice::read::repository::NamedMutexStorage storage;

  olp::dataservice::read::model::Partition partition;
  partition.SetDataHandle(kUrlBlobDataHandle);

  olp::client::HRN hrn(GetTestCatalog());
  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client, storage);

  std::promise<void> first_thread_finished_promise;
  std::promise<void> second_thread_finished_promise;

  // Start first request in a separate thread
  std::thread first_request_thread([&]() {
    auto response = repository.GetBlobData(
        kLayerId, kService, partition,
        olp::dataservice::read::FetchOptions::OnlineIfNotFound,
        olp::porting::none, context, false);

    EXPECT_FALSE(response);
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);

    first_thread_finished_promise.set_value();
  });

  // Start second request in a separate thread
  std::thread second_request_thread([&]() {
    auto response = repository.GetBlobData(
        kLayerId, kService, partition,
        olp::dataservice::read::FetchOptions::OnlineIfNotFound,
        olp::porting::none, context, false);

    EXPECT_FALSE(response);
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);

    second_thread_finished_promise.set_value();
  });

  const auto start = std::chrono::high_resolution_clock::now();

  // Start threads w/o wait for them to finish
  first_request_thread.detach();
  second_request_thread.detach();

  // Cancel operation should immeadeately finish both requests
  context.CancelOperation();

  // Wait until threads are finished
  first_thread_finished_promise.get_future().wait();
  second_thread_finished_promise.get_future().wait();

  const auto end = std::chrono::high_resolution_clock::now();

  // Compare time spanding for waiting for threads to finish
  EXPECT_LT(end - start, wait_time);
}

TEST_F(DataRepositoryTest, GetBlobDataFailedToCache) {
  auto cache_mock = std::make_shared<CacheMock>();

  settings_->propagate_all_cache_errors = true;
  settings_->cache = cache_mock;

  EXPECT_CALL(*cache_mock, Write(testing::HasSubstr("::api"), _, _))
      .WillRepeatedly(testing::Return(olp::client::ApiNoResult{}));
  EXPECT_CALL(*cache_mock, Write(testing::HasSubstr("::Data"), _, _))
      .WillRepeatedly(testing::Return(olp::client::ApiError::CacheIO()));

  EXPECT_CALL(*cache_mock, Get(_, _))
      .WillRepeatedly(testing::Return(olp::porting::any()));
  EXPECT_CALL(*cache_mock, Get(_)).WillRepeatedly(testing::Return(nullptr));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUrlResponseLookup));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlBlobData269), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  olp::client::CancellationContext context;

  olp::dataservice::read::model::Partition partition;
  partition.SetDataHandle(kUrlBlobDataHandle);

  olp::client::HRN hrn(GetTestCatalog());
  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client);
  auto response = repository.GetBlobData(
      kLayerId, kService, partition,
      olp::dataservice::read::FetchOptions::OnlineIfNotFound,
      olp::porting::none, context, true);

  ASSERT_FALSE(response);
  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::CacheIO);
}

TEST_F(DataRepositoryTest, GetVersionedDataTileFailedToCache) {
  auto cache_mock = std::make_shared<CacheMock>();

  settings_->propagate_all_cache_errors = true;
  settings_->cache = cache_mock;

  EXPECT_CALL(*cache_mock, Write(testing::HasSubstr("::api"), _, _))
      .WillRepeatedly(testing::Return(olp::client::ApiNoResult{}));
  EXPECT_CALL(*cache_mock, Write(testing::HasSubstr("::quadtree"), _, _))
      .WillRepeatedly(testing::Return(olp::client::ApiNoResult{}));
  EXPECT_CALL(*cache_mock, Write(testing::HasSubstr("::Data"), _, _))
      .WillRepeatedly(testing::Return(olp::client::ApiError::CacheIO()));

  EXPECT_CALL(*cache_mock, Get(_, _))
      .WillRepeatedly(testing::Return(olp::porting::any()));
  EXPECT_CALL(*cache_mock, Get(_)).WillRepeatedly(testing::Return(nullptr));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlLookup), _, _, _, _))
      .WillRepeatedly(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::OK),
                             kUrlResponseLookup));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kUrlQueryTreeIndex), _, _, _, _))
      .WillRepeatedly(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::OK),
                             kSubQuads));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(kUrlBlobData5904591), _, _, _, _))
      .WillRepeatedly(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::OK),
                             "someData"));

  olp::client::HRN hrn(GetTestCatalog());
  const auto kVersion = 4u;

  auto request = olp::dataservice::read::TileRequest().WithTileKey(
      olp::geo::TileKey::FromHereTile("5904591"));
  olp::client::CancellationContext context;
  ApiLookupClient lookup_client(hrn, *settings_);
  DataRepository repository(hrn, *settings_, lookup_client);
  auto response =
      repository.GetVersionedTile(kLayerId, request, kVersion, context);

  ASSERT_FALSE(response);
  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::CacheIO);
}
}  // namespace
