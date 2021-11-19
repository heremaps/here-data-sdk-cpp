/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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
#include <olp/authentication/TokenProvider.h>
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
#include <olp/dataservice/read/model/VersionInfos.h>
// clang-format off
#include "generated/serializer/CatalogSerializer.h"
#include "generated/serializer/VersionInfosSerializer.h"
#include "generated/serializer/JsonSerializer.h"
// clang-format on
#include "ReadDefaultResponses.h"
#include "ApiDefaultResponses.h"
#include "MockServerHelper.h"
#include "Utils.h"

namespace {

const auto kMockServerHost = "localhost";
const auto kMockServerPort = 1080;

const auto kAppId = "id";
const auto kAppSecret = "secret";
const auto kTestHrn = "hrn:here:data::olp-here-test:hereos-internal-test";
const auto kErrorMinVersion =
    R"jsonString({ "title": "Bad request", "status": 400,"detail": [{"name": "version", "error": "Invalid version: latest known version is 309"}]})jsonString";

const auto kMockRequestVersionsPath =
    "/metadata/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test/"
    "versions";
const auto kMockRequestCatalogPath =
    "/config/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test";

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

static olp::dataservice::read::model::VersionInfos GenerateVersionInfosResponse(
    std::int64_t start, std::int64_t end) {
  olp::dataservice::read::model::VersionInfos infos;
  auto size = end - start;
  if (size <= 0) {
    return infos;
  }
  std::vector<olp::dataservice::read::model::VersionInfo> versions_vect(size);
  for (size_t i = 0; i < versions_vect.size(); i++) {
    versions_vect[i].SetVersion(++start);
    versions_vect[i].SetTimestamp(1000 * start);
    std::vector<olp::dataservice::read::model::VersionDependency> dependencies(
        1);
    dependencies.front().SetHrn("hrn::some-value");
    versions_vect[i].SetDependencies(std::move(dependencies));
    versions_vect[i].SetPartitionCounts({{"partition", 1}});
  }

  infos.SetVersions(versions_vect);
  return infos;
}

static olp::dataservice::read::model::Catalog GenerateCatalogResponse() {
  olp::dataservice::read::model::Catalog catalog;
  catalog.SetHrn("hrn::some-value");
  catalog.SetVersion(1);
  return catalog;
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
    auth_client_settings.token_provider = provider;

    settings_ = olp::client::OlpClientSettings();
    settings_.network_request_handler = network;
    settings_.authentication_settings = auth_client_settings;
    olp::cache::CacheSettings cache_settings;
    settings_.cache = olp::client::OlpClientSettingsFactory::CreateDefaultCache(
        cache_settings);
    settings_.task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();

    // setup proxy
    settings_.proxy_settings =
        olp::http::NetworkProxySettings()
            .WithHostname(kMockServerHost)
            .WithPort(kMockServerPort)
            .WithType(olp::http::NetworkProxySettings::Type::HTTP);
    SetUpMockServer(network);
  }

  void TearDown() override {
    // very calls on mock server
    EXPECT_TRUE(mock_server_client_->Verify());
    mock_server_client_.reset();
  }

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
    mock_server_client_->MockLookupPlatformApiResponse(
        mockserver::ApiDefaultResponses::GeneratePlatformApisResponse());
    mock_server_client_->MockGetResponse(GenerateCatalogResponse(),
                                         kMockRequestCatalogPath);
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
}

TEST_F(CatalogClientTest, GetVersionsList) {
  const auto catalog = olp::client::HRN::FromString(kTestHrn);
  auto client = std::make_unique<olp::dataservice::read::CatalogClient>(
      catalog, settings_);
  {
    SCOPED_TRACE("Get versions list online");
    {
      mock_server_client_->MockAuth();
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
              kTestHrn));
      mock_server_client_->MockGetResponse(GenerateVersionInfosResponse(3, 4),
                                           kMockRequestVersionsPath);
    }

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
  }
  {
    SCOPED_TRACE("Get versions list error response");
    {
      mock_server_client_->MockLookupResourceApiResponse(
          mockserver::ApiDefaultResponses::GenerateResourceApisResponse(
              kTestHrn));
      mock_server_client_->MockGetError(
          {olp::http::HttpStatusCode::BAD_REQUEST, kErrorMinVersion},
          kMockRequestVersionsPath);
    }

    auto request = olp::dataservice::read::VersionsRequest()
                       .WithStartVersion(3)
                       .WithEndVersion(4);

    auto response_compressed =
        GetExecutionTime<olp::dataservice::read::VersionsResponse>([&] {
          auto future = client->ListVersions(request);
          return future.GetFuture().get();
        });

    EXPECT_FALSE(response_compressed.IsSuccessful());

    ASSERT_EQ(olp::client::ErrorCode::BadRequest,
              response_compressed.GetError().GetErrorCode());
    ASSERT_EQ(kErrorMinVersion, response_compressed.GetError().GetMessage());
  }
}

INSTANTIATE_TEST_SUITE_P(, CatalogClientTest,
                         ::testing::Values(CacheType::BOTH));

}  // namespace
