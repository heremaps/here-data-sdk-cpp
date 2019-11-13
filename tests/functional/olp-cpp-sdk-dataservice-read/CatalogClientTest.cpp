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
#include "Utils.h"

using namespace olp::dataservice::read;
using namespace testing;

namespace {

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

void DumpTileKey(const olp::geo::TileKey& tile_key) {
  std::cout << "Tile: " << tile_key.ToHereTile()
            << ", level: " << tile_key.Level()
            << ", parent: " << tile_key.Parent().ToHereTile() << std::endl;
}

class CatalogClientTest : public ::testing::TestWithParam<CacheType> {
 public:
  void SetUp() override {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();

    const auto key_id =
        CustomParameters::getArgument("dataservice_read_test_appid");
    const auto secret =
        CustomParameters::getArgument("dataservice_read_test_secret");

    olp::authentication::Settings authentication_settings({key_id, secret});
    authentication_settings.network_request_handler = network;

    olp::authentication::TokenProviderDefault provider(authentication_settings);
    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;
    settings_ = olp::client::OlpClientSettings();
    settings_.network_request_handler = network;
    settings_.authentication_settings = auth_client_settings;
    olp::cache::CacheSettings cache_settings;
    settings_.cache = olp::client::OlpClientSettingsFactory::CreateDefaultCache(
        cache_settings);
    client_ = olp::client::OlpClientFactory::Create(settings_);
  }

  void TearDown() override {
    client_.reset();
    auto network = std::move(settings_.network_request_handler);
    settings_ = olp::client::OlpClientSettings();
    // when test ends we must be sure that network pointer is not captured
    // anywhere
    EXPECT_EQ(network.use_count(), 1);
  }

 protected:
  std::string GetTestCatalog() {
    return CustomParameters::getArgument("dataservice_read_test_catalog");
  }

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
};

TEST_P(CatalogClientTest, GetCatalog) {
  olp::client::HRN hrn(GetTestCatalog());

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

INSTANTIATE_TEST_SUITE_P(, CatalogClientTest,
                         ::testing::Values(CacheType::BOTH));

}  // namespace
