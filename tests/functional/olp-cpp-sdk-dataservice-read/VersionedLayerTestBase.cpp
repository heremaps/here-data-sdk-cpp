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

#include "SetupMockServer.h"

VersionedLayerTestBase::VersionedLayerTestBase()
    : url_generator_(kTestHrn, kLayer) {}

void VersionedLayerTestBase::SetUp() {
  auto network = olp::client::OlpClientSettingsFactory::
      CreateDefaultNetworkRequestHandler();
  settings_ = mockserver::SetupMockServer::CreateSettings(network);
  mock_server_client_ =
      mockserver::SetupMockServer::CreateMockServer(network, kTestHrn);
}

void VersionedLayerTestBase::TearDown() {
  auto network = std::move(settings_->network_request_handler);
  settings_.reset();
  mock_server_client_.reset();
}
