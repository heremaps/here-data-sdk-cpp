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

#pragma once

#include <gmock/gmock.h>
#include <mocks/NetworkMock.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>

enum class CacheType { IN_MEMORY, DISK, BOTH };

class CatalogClientTestBase : public ::testing::TestWithParam<CacheType> {
 protected:
  CatalogClientTestBase();
  ~CatalogClientTestBase();
  std::string GetTestCatalog();
  static std::string ApiErrorToString(const olp::client::ApiError& error);

  void SetUp() override;
  void TearDown() override;

 private:
  void SetUpCommonNetworkMockCalls();

 protected:
  std::shared_ptr<olp::client::OlpClientSettings> settings_;
  std::shared_ptr<olp::client::OlpClient> client_;
  std::shared_ptr<NetworkMock> network_mock_;
};
