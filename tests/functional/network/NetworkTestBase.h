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

#include <gtest/gtest.h>

#include <memory>

#include <olp/core/http/Network.h>
#include <olp/core/http/NetworkSettings.h>

#include "Client.h"

class NetworkTestBase : public ::testing::Test {
  void SetUp() override;

 protected:
  std::shared_ptr<olp::http::Network> network_;
  olp::http::NetworkSettings settings_;
  std::shared_ptr<mockserver::Client> mock_server_client_;
};
