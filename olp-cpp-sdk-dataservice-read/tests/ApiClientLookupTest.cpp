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

#include "CacheMock.h"

#include "../src/ApiClientLookup.h"

using namespace olp;
using namespace client;
using namespace dataservice::read;

TEST(ApiClientLookup, LookupApiSync) {
  using namespace testing;
  using testing::Return;

  auto cache = std::make_shared<testing::StrictMock<CacheMock>>();
  // auto network = std::make_shared<testing::StrictMock<NetworkMock>>();

  const std::string catalog = "hrn:here:data:::hereos-internal-test-v2";

  const auto catalog_hrn = HRN::FromString(catalog);

  OlpClientSettings settings;
  settings.cache = cache;
  // settings.network_request_handler = network;

  const std::string service_name = "random_service";
  const std::string service_url = "http://random_service.com";
  const std::string service_version = "v8";
  const std::string cache_key =
      catalog + "::" + service_name + "::" + service_version + "::api";
  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] positive");
    EXPECT_CALL(*cache, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(service_url));
    client::CancellationContext context;

    auto response = ApiClientLookup::LookupApiSync(
        catalog_hrn, context, service_name, service_version,
        FetchOptions::CacheOnly, settings);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), service_url);
    Mock::VerifyAndClearExpectations(cache.get());
  }
  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] negative");
    EXPECT_CALL(*cache, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(boost::any()));
    client::CancellationContext context;

    auto response = ApiClientLookup::LookupApiSync(
        catalog_hrn, context, service_name, service_version,
        FetchOptions::CacheOnly, settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::NotFound);
    Mock::VerifyAndClearExpectations(cache.get());
  }
  {
      // SCOPED_TRACE("Fetch from network");
  } {
      // SCOPED_TRACE("Network error propagated to the user");
  } {
      // SCOPED_TRACE("Network request cancelled by the user");
  } {
    // SCOPED_TRACE("Network error propagated to the user");
  }
}