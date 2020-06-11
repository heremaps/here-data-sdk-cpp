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

#include <chrono>
#include <iostream>
#include <regex>
#include <string>
#include <tuple>

#include <gtest/gtest.h>

#include <testutils/CustomParameters.hpp>

#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenEndpoint.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/authentication/TokenResult.h>
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/Network.h>
#include <olp/core/http/NetworkRequest.h>
#include <olp/core/http/NetworkResponse.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/utils/Dir.h>
#include <olp/dataservice/read/CatalogClient.h>
#include <olp/dataservice/read/model/Catalog.h>
#include "DefaultResponses.h"
#include "MockServerHelper.h"
#include "Utils.h"

namespace {

const auto kMockServerHost = "localhost";
const auto kMockServerPort = 1080;

const auto kAppId = "id";
const auto kAppSecret = "secret";
const auto kTestHrn = "hrn:here:data::olp-here-test:hereos-internal-test";

enum class CacheType { IN_MEMORY, DISK, BOTH };

std::ostream& operator<<(std::ostream& os, const CacheType cache_type) {
  switch (cache_type) {
    case CacheType::IN_MEMORY:
      return os << "In-memory cache";
    case CacheType::DISK:
      return os << "Disk cache";
    case CacheType::BOTH:
      return os << "In-memory & disk cache";
    default:
      return os << "Unknown cache type";
  }
}

class CatalogClientTest : public ::testing::TestWithParam<CacheType> {
 public:
  void SetUp() override {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();

    olp::authentication::Settings auth_settings({kAppId, kAppSecret});
    auth_settings.network_request_handler = network;

    // setup proxy
    auth_settings.network_proxy_settings =
        olp::http::NetworkProxySettings()
            .WithHostname(kMockServerHost)
            .WithPort(kMockServerPort)
            .WithType(olp::http::NetworkProxySettings::Type::HTTP);
    olp::authentication::TokenProviderDefault provider(auth_settings);
    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;

    settings_ = olp::client::OlpClientSettings();
    settings_.network_request_handler = network;
    settings_.authentication_settings = auth_client_settings;
    olp::cache::CacheSettings cache_settings;
    settings_.cache = olp::client::OlpClientSettingsFactory::CreateDefaultCache(
        cache_settings);

    // setup proxy
    settings_.proxy_settings =
        olp::http::NetworkProxySettings()
            .WithHostname(kMockServerHost)
            .WithPort(kMockServerPort)
            .WithType(olp::http::NetworkProxySettings::Type::HTTP);
    SetUpMockServer(network);
  }

  void TearDown() override { mock_server_client_.reset(); }

  void SetUpMockServer(std::shared_ptr<olp::http::Network> network) {
    // create client to set mock server expectations
    olp::client::OlpClientSettings olp_client_settings;
    olp_client_settings.network_request_handler = network;
    mock_server_client_ = std::make_shared<mockserver::MockServerHelper>(
        olp_client_settings, kTestHrn);
  }

 protected:
  std::string GetTestCatalog() { return kTestHrn; }

  template <typename T>
  T GetExecutionTime(std::function<T()> func) {
    auto start_time = std::chrono::high_resolution_clock::now();
    auto result = func();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time = end - start_time;
    std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;

    return result;
  }

 protected:
  olp::client::OlpClientSettings settings_;
  std::shared_ptr<olp::client::OlpClient> client_;
  std::shared_ptr<mockserver::MockServerHelper> mock_server_client_;
};

TEST_P(CatalogClientTest, GetCatalog) {
  olp::client::HRN hrn(kTestHrn);
  {
    mock_server_client_->MockAuth();
    mock_server_client_->MockTimestamp();
    mock_server_client_->MockGetPlatformApiResponse(
        mockserver::DefaultResponses::GeneratePlatformApisResponse());
    mock_server_client_->MockGetResponse(
        mockserver::DefaultResponses::GenerateCatalogResponse());
  }

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::CatalogRequest();

  auto catalog_response =
      GetExecutionTime<olp::dataservice::read::CatalogResponse>([&] {
        auto future = catalog_client->GetCatalog(request);
        return future.GetFuture().get();
      });

  EXPECT_SUCCESS(catalog_response);
  EXPECT_TRUE(mock_server_client_->Verify());
}

TEST_P(CatalogClientTest, GetVersionsList) {
  const auto catalog = olp::client::HRN::FromString(kTestHrn);
  {
    mock_server_client_->MockAuth();
    mock_server_client_->MockTimestamp();
    mock_server_client_->MockGetResponse(
        mockserver::DefaultResponses::GenerateResourceApisResponse(kTestHrn));
    mock_server_client_->MockGetResponse(
        mockserver::DefaultResponses::GenerateVersionInfosResponse(3, 4));
  }

  auto client = std::make_unique<olp::dataservice::read::CatalogClient>(
      catalog, settings_);
  {
    SCOPED_TRACE("Get versions list online");
    auto request = olp::dataservice::read::VersionsRequest()
                       .WithStartVersion(3)
                       .WithEndVersion(4);

    auto response_compressed =
        GetExecutionTime<olp::dataservice::read::VersionsResponse>([&] {
          auto future = client->ListVersions(request);
          return future.GetFuture().get();
        });

    EXPECT_SUCCESS(response_compressed);
    ASSERT_EQ(1u, response_compressed.GetResult().GetVersions().size());
    ASSERT_EQ(
        4, response_compressed.GetResult().GetVersions().front().GetVersion());
    ASSERT_EQ(1u, response_compressed.GetResult()
                      .GetVersions()
                      .front()
                      .GetDependencies()
                      .size());
    ASSERT_EQ(1u, response_compressed.GetResult()
                      .GetVersions()
                      .front()
                      .GetPartitionCounts()
                      .size());
    // very calls on mock server
    EXPECT_TRUE(mock_server_client_->Verify());
  }
  {
    SCOPED_TRACE("Get versions list from cache");

    auto request =
        olp::dataservice::read::VersionsRequest()
            .WithStartVersion(3)
            .WithEndVersion(4)
            .WithFetchOption(olp::dataservice::read::FetchOptions::CacheOnly);

    auto response_compressed =
        GetExecutionTime<olp::dataservice::read::VersionsResponse>([&] {
          auto future = client->ListVersions(request);
          return future.GetFuture().get();
        });
    ASSERT_FALSE(response_compressed.IsSuccessful());

    ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
              response_compressed.GetError().GetErrorCode());
  }
}

INSTANTIATE_TEST_SUITE_P(, CatalogClientTest,
                         ::testing::Values(CacheType::BOTH));

}  // namespace
