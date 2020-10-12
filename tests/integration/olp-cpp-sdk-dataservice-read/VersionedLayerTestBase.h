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

#pragma once

#include <gmock/gmock.h>
#include <mocks/NetworkMock.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/dataservice/read/VersionedLayerClient.h>

#include "PlatformUrlsGenerator.h"
#include "ReadDefaultResponses.h"

class VersionedLayerTestBase : public ::testing::Test {
 public:
  VersionedLayerTestBase();

  void SetUp() override;

  void TearDown() override;

  void ExpectQuadTreeRequest(int64_t version,
                             mockserver::QuadTreeBuilder quad_tree,
                             olp::http::NetworkResponse response =
                                 olp::http::NetworkResponse().WithStatus(
                                     olp::http::HttpStatusCode::OK));

  void ExpectBlobRequest(const std::string& data_handle,
                         const std::string& data,
                         olp::http::NetworkResponse response =
                             olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::OK));

  void ExpectVersionRequest(olp::http::NetworkResponse response =
                                olp::http::NetworkResponse().WithStatus(
                                    olp::http::HttpStatusCode::OK));
  void ExpectQueryPartitionsRequest(
      const std::vector<std::string>& partitions,
      const olp::dataservice::read::model::Partitions& partitions_response,
      olp::http::NetworkResponse response =
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK));

 protected:
  const std::string kCatalog = "hrn:here:data::olp-here-test:catalog";
  const std::string kLayerName = "testlayer";
  const olp::client::HRN kCatalogHrn = olp::client::HRN::FromString(kCatalog);
  const std::string kEndpoint = "https://localhost";

  olp::client::OlpClientSettings settings_;
  std::shared_ptr<NetworkMock> network_mock_;
  PlatformUrlsGenerator url_generator_;
  const uint32_t version_ = 4;
};
