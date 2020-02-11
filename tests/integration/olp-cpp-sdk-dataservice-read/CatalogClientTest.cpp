/*
 * Copyright (C) 2019 HERE Europe B.V.
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
#include <olp/core/client/HRN.h>
#include <olp/core/http/Network.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/dataservice/read/CatalogClient.h>
#include <olp/dataservice/read/model/Catalog.h>

#include "CatalogClientTestBase.h"
#include "HttpResponses.h"

namespace {

using namespace olp::dataservice::read;
using namespace testing;
using namespace olp::tests::common;
using namespace olp::tests::integration;

class CatalogClientTest : public CatalogClientTestBase {};

TEST_P(CatalogClientTest, GetCatalog) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);
  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  auto future = catalog_client->GetCatalog(request);
  CatalogResponse catalog_response = future.GetFuture().get();

  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
}

TEST_P(CatalogClientTest, GetCatalogCallback) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogRequest();

  std::promise<CatalogResponse> promise;
  CatalogResponseCallback callback = [&promise](CatalogResponse response) {
    promise.set_value(response);
  };
  catalog_client->GetCatalog(request, callback);
  CatalogResponse catalog_response = promise.get_future().get();
  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
}

TEST_P(CatalogClientTest, GetCatalog403) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(403),
                                   HTTP_RESPONSE_403));

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  auto future = catalog_client->GetCatalog(request);
  CatalogResponse catalog_response = future.GetFuture().get();

  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
  ASSERT_EQ(403, catalog_response.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientTest, GetCatalogCancelApiLookup) {
  olp::client::HRN hrn(GetTestCatalog());

  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_CONFIG});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(0);

  // Run it!
  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogRequest();

  std::promise<CatalogResponse> promise;
  CatalogResponseCallback callback = [&promise](CatalogResponse response) {
    promise.set_value(response);
  };
  olp::client::CancellationToken cancel_token =
      catalog_client->GetCatalog(request, callback);

  wait_for_cancel->get_future().get();
  cancel_token.Cancel();
  pause_for_cancel->set_value();
  CatalogResponse catalog_response = promise.get_future().get();

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
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_CONFIG});

  // Setup the expected calls :
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  // Run it!
  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogRequest();

  std::promise<CatalogResponse> promise;
  CatalogResponseCallback callback = [&promise](CatalogResponse response) {
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
  CatalogResponse catalog_response = promise.get_future().get();

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
  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogRequest();

  std::promise<CatalogResponse> promise;
  CatalogResponseCallback callback = [&promise](CatalogResponse response) {
    promise.set_value(response);
  };
  olp::client::CancellationToken cancel_token =
      catalog_client->GetCatalog(request, callback);

  CatalogResponse catalog_response = promise.get_future().get();

  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());

  cancel_token.Cancel();
}

TEST_P(CatalogClientTest, GetCatalogVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1);

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogVersionRequest().WithStartVersion(-1);

  auto future = catalog_client->GetLatestVersion(request);
  auto catalog_version_response = future.GetFuture().get();

  ASSERT_TRUE(catalog_version_response.IsSuccessful())
      << ApiErrorToString(catalog_version_response.GetError());
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
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_METADATA});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  // Run it!
  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogVersionRequest().WithStartVersion(-1);

  std::promise<CatalogVersionResponse> promise;
  CatalogVersionCallback callback =
      [&promise](CatalogVersionResponse response) {
        promise.set_value(response);
      };
  olp::client::CancellationToken cancel_token =
      catalog_client->GetLatestVersion(request, callback);

  wait_for_cancel->get_future().get();
  cancel_token.Cancel();
  pause_for_cancel->set_value();
  CatalogVersionResponse version_response = promise.get_future().get();

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

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  request.WithFetchOption(CacheOnly);
  auto future = catalog_client->GetCatalog(request);
  CatalogResponse catalog_response = future.GetFuture().get();
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
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(429),
                               "Server busy at the moment."));
  }

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  request.WithFetchOption(OnlineOnly);
  auto future = catalog_client->GetCatalog(request);
  CatalogResponse catalog_response = future.GetFuture().get();
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

  std::tie(request_id, send_mock, cancel_mock) =
      GenerateNetworkMockActions(wait_to_start_signal, pre_callback_wait,
                                 {200, HTTP_RESPONSE_CONFIG}, wait_for_end);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  request.WithFetchOption(CacheWithUpdate);
  // Request 1
  SCOPED_TRACE("Request Catalog, CacheWithUpdate");
  auto future = catalog_client->GetCatalog(request);

  SCOPED_TRACE("get CatalogResponse1");
  CatalogResponse catalog_response = future.GetFuture().get();

  // Request 1 return. Cached value (nothing)
  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
  // Wait for background cache update to finish
  SCOPED_TRACE("wait some time for update to conclude");
  wait_for_end->get_future().get();

  // Request 2 to check there is a cached value.
  SCOPED_TRACE("Request Catalog, CacheOnly");
  request.WithFetchOption(CacheOnly);
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
            olp::http::NetworkResponse().WithStatus(403), HTTP_RESPONSE_403));
  }

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  // Populate cache
  auto future = catalog_client->GetCatalog(request);
  CatalogResponse catalog_response = future.GetFuture().get();
  ASSERT_TRUE(catalog_response.IsSuccessful());
  // Receive 403
  request.WithFetchOption(OnlineOnly);
  future = catalog_client->GetCatalog(request);
  catalog_response = future.GetFuture().get();
  ASSERT_FALSE(catalog_response.IsSuccessful());
  ASSERT_EQ(403, catalog_response.GetError().GetHttpStatusCode());
  // Check for cached response
  request.WithFetchOption(CacheOnly);
  future = catalog_client->GetCatalog(request);
  catalog_response = future.GetFuture().get();
  ASSERT_FALSE(catalog_response.IsSuccessful());
}

TEST_P(CatalogClientTest, CancelPendingRequestsCatalog) {
  olp::client::HRN hrn(GetTestCatalog());
  testing::InSequence s;
  std::vector<std::shared_ptr<std::promise<void>>> pauses;

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto catalog_request = CatalogRequest().WithFetchOption(OnlineOnly);
  auto version_request = CatalogVersionRequest().WithFetchOption(OnlineOnly);

  // Make a few requests
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_CONFIG});

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
  CatalogResponse catalog_response = catalog_future.GetFuture().get();
  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            catalog_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            catalog_response.GetError().GetErrorCode());

  CatalogVersionResponse version_response = version_future.GetFuture().get();

  ASSERT_FALSE(version_response.IsSuccessful())
      << ApiErrorToString(version_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            version_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            version_response.GetError().GetErrorCode());
}

INSTANTIATE_TEST_SUITE_P(, CatalogClientTest,
                         ::testing::Values(CacheType::BOTH));

}  // namespace
