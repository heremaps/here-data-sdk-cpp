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

#include "VersionedLayerTestBase.h"

#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/utils/Dir.h>

#include "matchers/NetworkUrlMatchers.h"

namespace {
const auto kCachePathMutable = "./tmp_cache";
}

using testing::_;

void VersionedLayerTestBase::SetUp() {
  olp::utils::Dir::remove(kCachePathMutable);

  network_mock_ = std::make_shared<NetworkMock>();

  settings_.api_lookup_settings.catalog_endpoint_provider =
      [=](const olp::client::HRN&) { return kEndpoint; };

  settings_.network_request_handler = network_mock_;
  settings_.task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  olp::cache::CacheSettings settings;
  settings.disk_path_mutable = kCachePathMutable;
  settings_.cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache(settings);
}

void VersionedLayerTestBase::TearDown() {
  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  network_mock_.reset();
  settings_.task_scheduler.reset();
  olp::utils::Dir::remove(kCachePathMutable);
}

void VersionedLayerTestBase::ExpectQuadTreeRequest(
    const std::string& layer, int64_t version,
    mockserver::QuadTreeBuilder quad_tree) {
  std::stringstream url;
  url << kEndpoint << "/catalogs/" << kCatalog << "/layers/" << layer
      << "/versions/" << version << "/quadkeys/"
      << quad_tree.Root().ToHereTile() << "/depths/4";

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(url.str()), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   quad_tree.BuildJson()));
}

void VersionedLayerTestBase::ExpectBlobRequest(const std::string& layer,
                                               const std::string& data_handle,
                                               const std::string& data) {
  std::stringstream url;
  url << kEndpoint << "/catalogs/" << kCatalog << "/layers/" << layer
      << "/data/" << data_handle;

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(url.str()), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   data));
}
