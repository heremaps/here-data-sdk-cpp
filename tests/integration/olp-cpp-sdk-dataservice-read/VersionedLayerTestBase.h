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

#include <olp/dataservice/read/VersionedLayerClient.h>

#include <mocks/NetworkMock.h>

#include <olp/core/http/HttpStatusCode.h>

#include "ReadDefaultResponses.h"

class VersionedLayerTestBase : public ::testing::Test {
 public:
  void SetUp() override;

  void TearDown() override;

  void ExpectQuadTreeRequest(const std::string& layer, int64_t version,
                             mockserver::QuadTreeBuilder quad_tree);

  void ExpectBlobRequest(const std::string& layer,
                         const std::string& data_handle,
                         const std::string& data);

 protected:
  const std::string kCatalog = "hrn:here:data::olp-here-test:catalog";
  const olp::client::HRN kCatalogHrn = olp::client::HRN::FromString(kCatalog);
  const std::string kEndpoint = "https://localhost";

  olp::client::OlpClientSettings settings_;
  std::shared_ptr<NetworkMock> network_mock_;
};
