/*
 * Copyright (C) 2019-2022 HERE Europe B.V.
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

#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/NetworkMock.h>
#include <olp/core/cache/KeyGenerator.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/HRN.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/Network.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/dataservice/read/CatalogClient.h>
#include <olp/dataservice/read/model/Catalog.h>

#include "CatalogClientTestBase.h"
#include "HttpResponses.h"

namespace {
constexpr auto kStartVersion = 3;
constexpr auto kEndVersion = 4;
constexpr auto kCompatibleVersions =
    "https://metadata.data.api.platform.here.com/metadata/v1/catalogs/"
    "hereos-internal-test-v2/versions/compatibles?limit=100";
constexpr auto kUrlVersionsList =
    R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/versions?endVersion=4&startVersion=3)";
constexpr auto kUrlVersionsListStartMinus =
    R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/versions?endVersion=4&startVersion=-1)";
constexpr auto kHttpVersionsListResponse =
    R"jsonString({"versions":[{"version":4,"timestamp":1547159598712,"partitionCounts":{"testlayer":5,"testlayer_res":1,"multilevel_testlayer":33, "hype-test-prefetch-2":7,"testlayer_gzip":1,"hype-test-prefetch":7},"dependencies":[ { "hrn":"hrn:here:data::olp-here-test:hereos-internal-test-v2","version":0,"direct":false},{"hrn":"hrn:here:data:::hereos-internal-test-v2","version":0,"direct":false }]}]})jsonString";
constexpr auto kCompatibleVersionsResponse =
    R"jsonString({"versions":[{"version":30,"sharedDependencies":[{"hrn":"test","version":15}]},{"version":29,"sharedDependencies":[]}]})jsonString";

using testing::_;
namespace read = olp::dataservice::read;
namespace http = olp::http;

class CatalogClientTest
    : public olp::tests::integration::CatalogClientTestBase {};

TEST_P(CatalogClientTest, GetCatalog) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);
  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);
  auto request = read::CatalogRequest();
  auto future = catalog_client->GetCatalog(request);
  read::CatalogResponse catalog_response = future.GetFuture().get();

  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());

  const auto cache_key =
      olp::cache::KeyGenerator::CreateCatalogKey(GetTestCatalog());
  EXPECT_TRUE(settings_.cache->Contains(cache_key));
}

TEST_P(CatalogClientTest, GetCatalogCallback) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);

  auto request = read::CatalogRequest();

  std::promise<read::CatalogResponse> promise;
  read::CatalogResponseCallback callback =
      [&promise](read::CatalogResponse response) {
        promise.set_value(response);
      };
  catalog_client->GetCatalog(request, callback);
  read::CatalogResponse catalog_response = promise.get_future().get();
  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
}

TEST_P(CatalogClientTest, GetCatalog403) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .WillOnce(ReturnHttpResponse(GetResponse(http::HttpStatusCode::FORBIDDEN),
                                   HTTP_RESPONSE_403));

  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);
  auto request = read::CatalogRequest();
  auto future = catalog_client->GetCatalog(request);
  read::CatalogResponse catalog_response = future.GetFuture().get();

  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
  ASSERT_EQ(http::HttpStatusCode::FORBIDDEN,
            catalog_response.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientTest, GetCatalogCancelApiLookup) {
  olp::client::HRN hrn(GetTestCatalog());

  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_CONFIG});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(0);

  // Run it!
  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);

  auto request = read::CatalogRequest();

  std::promise<read::CatalogResponse> promise;
  read::CatalogResponseCallback callback =
      [&promise](read::CatalogResponse response) {
        promise.set_value(response);
      };
  olp::client::CancellationToken cancel_token =
      catalog_client->GetCatalog(request, callback);

  wait_for_cancel->get_future().get();
  cancel_token.Cancel();
  pause_for_cancel->set_value();
  read::CatalogResponse catalog_response = promise.get_future().get();

  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            catalog_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            catalog_response.GetError().GetErrorCode());
}

TEST_P(CatalogClientTest, GetCatalogCancelConfig) {
  olp::client::HRN hrn(GetTestCatalog());

  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_CONFIG});

  // Setup the expected calls :
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  // Run it!
  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);

  auto request = read::CatalogRequest();

  std::promise<read::CatalogResponse> promise;
  read::CatalogResponseCallback callback =
      [&promise](read::CatalogResponse response) {
        promise.set_value(response);
      };
  olp::client::CancellationToken cancel_token =
      catalog_client->GetCatalog(request, callback);

  wait_for_cancel->get_future().get();
  std::cout << "Cancelling" << std::endl;
  cancel_token.Cancel();  // crashing?
  std::cout << "Cancelled, unblocking response" << std::endl;
  pause_for_cancel->set_value();
  std::cout << "Post Cancel, get response" << std::endl;
  read::CatalogResponse catalog_response = promise.get_future().get();

  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            catalog_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            catalog_response.GetError().GetErrorCode());
  std::cout << "Post Test" << std::endl;
}

TEST_P(CatalogClientTest, GetCatalogCancelAfterCompletion) {
  olp::client::HRN hrn(GetTestCatalog());

  // Run it!
  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);

  auto request = read::CatalogRequest();

  std::promise<read::CatalogResponse> promise;
  read::CatalogResponseCallback callback =
      [&promise](read::CatalogResponse response) {
        promise.set_value(response);
      };
  olp::client::CancellationToken cancel_token =
      catalog_client->GetCatalog(request, callback);

  read::CatalogResponse catalog_response = promise.get_future().get();

  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());

  cancel_token.Cancel();
}

TEST_P(CatalogClientTest, GetCatalogVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1);

  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);

  auto request = read::CatalogVersionRequest().WithStartVersion(-1);

  auto future = catalog_client->GetLatestVersion(request);
  auto catalog_version_response = future.GetFuture().get();

  ASSERT_TRUE(catalog_version_response.IsSuccessful())
      << ApiErrorToString(catalog_version_response.GetError());

  const auto cache_key =
      olp::cache::KeyGenerator::CreateLatestVersionKey(GetTestCatalog());
  EXPECT_TRUE(settings_.cache->Contains(cache_key));
}

TEST_P(CatalogClientTest, GetCatalogVersionCancel) {
  olp::client::HRN hrn(GetTestCatalog());

  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  // Setup the expected calls :
  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  // Run it!
  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);

  auto request = read::CatalogVersionRequest().WithStartVersion(-1);

  std::promise<read::CatalogVersionResponse> promise;
  read::CatalogVersionCallback callback =
      [&promise](read::CatalogVersionResponse response) {
        promise.set_value(response);
      };
  olp::client::CancellationToken cancel_token =
      catalog_client->GetLatestVersion(request, callback);

  wait_for_cancel->get_future().get();
  cancel_token.Cancel();
  pause_for_cancel->set_value();
  read::CatalogVersionResponse version_response = promise.get_future().get();

  ASSERT_FALSE(version_response.IsSuccessful())
      << ApiErrorToString(version_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            version_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            version_response.GetError().GetErrorCode());
}

TEST_P(CatalogClientTest, GetCatalogCacheOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(0);

  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);
  auto request = read::CatalogRequest();
  request.WithFetchOption(read::CacheOnly);
  auto future = catalog_client->GetCatalog(request);
  read::CatalogResponse catalog_response = future.GetFuture().get();
  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
}

TEST_P(CatalogClientTest, GetCatalogOnlineOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .Times(4)
        .WillRepeatedly(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));
  }

  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);
  auto request = read::CatalogRequest();
  request.WithFetchOption(read::OnlineOnly);
  auto future = catalog_client->GetCatalog(request);
  read::CatalogResponse catalog_response = future.GetFuture().get();
  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
  future = catalog_client->GetCatalog(request);
  // Should fail despite valid cache entry.
  catalog_response = future.GetFuture().get();
  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
}

TEST_P(CatalogClientTest, GetCatalogCacheWithUpdate) {
  olp::logging::Log::setLevel(olp::logging::Level::Trace);

  olp::client::HRN hrn(GetTestCatalog());
  auto wait_to_start_signal = std::make_shared<std::promise<void>>();
  auto pre_callback_wait = std::make_shared<std::promise<void>>();
  pre_callback_wait->set_value();
  auto wait_for_end = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_to_start_signal, pre_callback_wait,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_CONFIG}, wait_for_end);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);
  auto request = read::CatalogRequest();
  request.WithFetchOption(read::CacheWithUpdate);
  // Request 1
  SCOPED_TRACE("Request Catalog, CacheWithUpdate");
  auto future = catalog_client->GetCatalog(request);

  SCOPED_TRACE("get CatalogResponse1");
  read::CatalogResponse catalog_response = future.GetFuture().get();

  // Request 1 return. Cached value (nothing)
  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
  // Wait for background cache update to finish
  SCOPED_TRACE("wait some time for update to conclude");
  wait_for_end->get_future().get();

  // Request 2 to check there is a cached value.
  SCOPED_TRACE("Request Catalog, CacheOnly");
  request.WithFetchOption(read::CacheOnly);
  future = catalog_client->GetCatalog(request);
  SCOPED_TRACE("get CatalogResponse2");
  catalog_response = future.GetFuture().get();
  // Cache should be available here.
  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
}

TEST_P(CatalogClientTest, GetCatalog403CacheClear) {
  olp::client::HRN hrn(GetTestCatalog());
  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::FORBIDDEN), HTTP_RESPONSE_403));
  }

  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);
  auto request = read::CatalogRequest();
  // Populate cache
  auto future = catalog_client->GetCatalog(request);
  read::CatalogResponse catalog_response = future.GetFuture().get();
  ASSERT_TRUE(catalog_response.IsSuccessful());
  // Receive 403
  request.WithFetchOption(read::OnlineOnly);
  future = catalog_client->GetCatalog(request);
  catalog_response = future.GetFuture().get();
  ASSERT_FALSE(catalog_response.IsSuccessful());
  ASSERT_EQ(http::HttpStatusCode::FORBIDDEN,
            catalog_response.GetError().GetHttpStatusCode());
  // Check for cached response
  request.WithFetchOption(read::CacheOnly);
  future = catalog_client->GetCatalog(request);
  catalog_response = future.GetFuture().get();
  ASSERT_FALSE(catalog_response.IsSuccessful());
}

TEST_P(CatalogClientTest, CancelPendingRequestsCatalog) {
  olp::client::HRN hrn(GetTestCatalog());
  testing::InSequence s;
  std::vector<std::shared_ptr<std::promise<void>>> pauses;

  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);
  auto catalog_request =
      read::CatalogRequest().WithFetchOption(read::OnlineOnly);
  auto version_request =
      read::CatalogVersionRequest().WithFetchOption(read::OnlineOnly);

  // Make a few requests
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        wait_for_cancel, pause_for_cancel,
        {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_CONFIG});

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }

  auto catalog_future = catalog_client->GetCatalog(catalog_request);
  auto version_future = catalog_client->GetLatestVersion(version_request);

  // We are using only one thread so we can only have one network request
  // active. So just wait for it.
  wait_for_cancel->get_future().get();

  // Cancel them all
  catalog_client->CancelPendingRequests();
  pause_for_cancel->set_value();

  // Verify they are all cancelled
  read::CatalogResponse catalog_response = catalog_future.GetFuture().get();
  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            catalog_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            catalog_response.GetError().GetErrorCode());

  read::CatalogVersionResponse version_response =
      version_future.GetFuture().get();

  ASSERT_FALSE(version_response.IsSuccessful())
      << ApiErrorToString(version_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            version_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            version_response.GetError().GetErrorCode());
}

TEST_F(CatalogClientTest, GetVersionsList) {
  olp::client::HRN catalog(GetTestCatalog());

  auto client = olp::dataservice::read::CatalogClient(catalog, settings_);
  {
    SCOPED_TRACE("Get versions list online");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kUrlVersionsList), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kHttpVersionsListResponse));

    auto request = olp::dataservice::read::VersionsRequest()
                       .WithStartVersion(kStartVersion)
                       .WithEndVersion(kEndVersion);

    auto future = client.ListVersions(request);
    auto response = future.GetFuture().get();

    EXPECT_TRUE(response.IsSuccessful());
    ASSERT_EQ(1u, response.GetResult().GetVersions().size());
    ASSERT_EQ(4, response.GetResult().GetVersions().front().GetVersion());
    ASSERT_EQ(
        2u,
        response.GetResult().GetVersions().front().GetDependencies().size());
    ASSERT_EQ(
        6u,
        response.GetResult().GetVersions().front().GetPartitionCounts().size());
  }
  {
    SCOPED_TRACE("Get versions list start version -1");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kUrlVersionsListStartMinus), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kHttpVersionsListResponse));

    auto request = olp::dataservice::read::VersionsRequest()
                       .WithStartVersion(-1)
                       .WithEndVersion(kEndVersion);

    auto future = client.ListVersions(request);
    auto response = future.GetFuture().get();

    EXPECT_TRUE(response.IsSuccessful());
    ASSERT_EQ(1u, response.GetResult().GetVersions().size());
    ASSERT_EQ(4, response.GetResult().GetVersions().front().GetVersion());
    ASSERT_EQ(
        2u,
        response.GetResult().GetVersions().front().GetDependencies().size());
    ASSERT_EQ(
        6u,
        response.GetResult().GetVersions().front().GetPartitionCounts().size());
  }
  {
    SCOPED_TRACE("Get versions list error");

    auto request = olp::dataservice::read::VersionsRequest()
                       .WithStartVersion(kStartVersion)
                       .WithEndVersion(kEndVersion);

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kUrlVersionsList), _, _, _, _))
        .Times(4)
        .WillRepeatedly(ReturnHttpResponse(
            GetResponse(http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));

    auto future = client.ListVersions(request);
    auto response = future.GetFuture().get();

    ASSERT_FALSE(response.IsSuccessful())
        << ApiErrorToString(response.GetError());

    ASSERT_EQ(static_cast<int>(olp::http::HttpStatusCode::TOO_MANY_REQUESTS),
              response.GetError().GetHttpStatusCode());
  }
}

TEST_F(CatalogClientTest, GetVersionsListCancel) {
  olp::client::HRN hrn(GetTestCatalog());

  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlVersionsList), _, _, _, _))
      .Times(0);

  auto catalog_client = read::CatalogClient(hrn, settings_);

  auto request = read::VersionsRequest()
                     .WithStartVersion(kStartVersion)
                     .WithEndVersion(kEndVersion);

  std::promise<read::VersionsResponse> promise;
  read::VersionsResponseCallback callback =
      [&promise](read::VersionsResponse response) {
        promise.set_value(response);
      };

  auto cancel_token = catalog_client.ListVersions(request, callback);

  wait_for_cancel->get_future().get();
  cancel_token.Cancel();
  pause_for_cancel->set_value();
  auto versions_response = promise.get_future().get();

  ASSERT_FALSE(versions_response.IsSuccessful())
      << ApiErrorToString(versions_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            versions_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            versions_response.GetError().GetErrorCode());
}

TEST_F(CatalogClientTest, GetVersionsListCallback) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrlVersionsList), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kHttpVersionsListResponse));

  auto catalog_client = read::CatalogClient(hrn, settings_);

  auto request = read::VersionsRequest()
                     .WithStartVersion(kStartVersion)
                     .WithEndVersion(kEndVersion);

  std::promise<read::VersionsResponse> promise;
  read::VersionsResponseCallback callback =
      [&promise](read::VersionsResponse response) {
        promise.set_value(response);
      };

  auto cancel_token = catalog_client.ListVersions(request, callback);

  auto response = promise.get_future().get();

  EXPECT_TRUE(response.IsSuccessful());
  ASSERT_EQ(1u, response.GetResult().GetVersions().size());
  ASSERT_EQ(4, response.GetResult().GetVersions().front().GetVersion());
  ASSERT_EQ(
      2u, response.GetResult().GetVersions().front().GetDependencies().size());
  ASSERT_EQ(
      6u,
      response.GetResult().GetVersions().front().GetPartitionCounts().size());
}

TEST_F(CatalogClientTest, GetCompatibleVersionsCallback) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    SCOPED_TRACE("Normal call");

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(kCompatibleVersions), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kCompatibleVersionsResponse));

    auto catalog_client = read::CatalogClient(hrn, settings_);

    auto dependency = read::CompatibleVersionDependency();
    dependency.SetHrn("test");
    dependency.SetVersion(15);

    auto request =
        read::CompatibleVersionsRequest().WithDependencies({dependency});

    std::promise<read::CompatibleVersionsResponse> promise;
    read::CompatibleVersionsCallback callback =
        [&promise](read::CompatibleVersionsResponse response) {
          promise.set_value(response);
        };

    auto cancel_token = catalog_client.GetCompatibleVersions(request, callback);

    auto response = promise.get_future().get();

    ASSERT_TRUE(response.IsSuccessful());
    ASSERT_FALSE(response.GetResult().GetVersionInfos().empty());

    auto version_info = response.GetResult().GetVersionInfos()[0];

    EXPECT_EQ(version_info.version, 30);
    ASSERT_FALSE(version_info.dependencies.empty());
    EXPECT_EQ(version_info.dependencies[0].GetHrn(), "test");
    EXPECT_EQ(version_info.dependencies[0].GetVersion(), 15);
  }
  {
    SCOPED_TRACE("Request failed");
    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(kCompatibleVersions), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::FORBIDDEN),
                                     HTTP_RESPONSE_403));

    auto catalog_client = read::CatalogClient(hrn, settings_);

    auto dependency = read::CompatibleVersionDependency();
    dependency.SetHrn("test");
    dependency.SetVersion(15);

    auto request =
        read::CompatibleVersionsRequest().WithDependencies({dependency});

    std::promise<read::CompatibleVersionsResponse> promise;
    read::CompatibleVersionsCallback callback =
        [&promise](read::CompatibleVersionsResponse response) {
          promise.set_value(response);
        };

    auto cancel_token = catalog_client.GetCompatibleVersions(request, callback);

    auto response = promise.get_future().get();

    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              olp::http::HttpStatusCode::FORBIDDEN);
  }
  {
    SCOPED_TRACE("Request cancel");
    auto wait_for_cancel = std::make_shared<std::promise<void>>();
    auto pause_for_cancel = std::make_shared<std::promise<void>>();

    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        wait_for_cancel, pause_for_cancel,
        {http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(kUrlVersionsList), _, _, _, _))
        .Times(0);

    auto catalog_client = read::CatalogClient(hrn, settings_);

    auto dependency = read::CompatibleVersionDependency();
    dependency.SetHrn("test");
    dependency.SetVersion(15);

    auto request =
        read::CompatibleVersionsRequest().WithDependencies({dependency});

    std::promise<read::CompatibleVersionsResponse> promise;
    read::CompatibleVersionsCallback callback =
        [&promise](read::CompatibleVersionsResponse response) {
          promise.set_value(response);
        };

    auto cancel_token = catalog_client.GetCompatibleVersions(request, callback);

    wait_for_cancel->get_future().get();
    cancel_token.Cancel();
    pause_for_cancel->set_value();
    auto response = promise.get_future().get();

    ASSERT_FALSE(response.IsSuccessful())
        << ApiErrorToString(response.GetError());

    ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
              response.GetError().GetHttpStatusCode());
    ASSERT_EQ(olp::client::ErrorCode::Cancelled,
              response.GetError().GetErrorCode());
  }
}

INSTANTIATE_TEST_SUITE_P(
    , CatalogClientTest,
    ::testing::Values(olp::tests::integration::CacheType::BOTH));

}  // namespace
