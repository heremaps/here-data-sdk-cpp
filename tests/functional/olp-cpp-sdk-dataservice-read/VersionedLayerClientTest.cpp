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

#include <gtest/gtest.h>
#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/NetworkSettings.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
#include <string>
#include <testutils/CustomParameters.hpp>
#include "DefaultResponses.h"
#include "MockServerHelper.h"
#include "Utils.h"

namespace {

const auto kMockServerHost = "localhost";
const auto kMockServerPort = 1080;

const auto kAppId = "id";
const auto kAppSecret = "secret";
const auto kTestHrn = "hrn:here:data::olp-here-test:hereos-internal-test";

class VersionedLayerClientTest : public ::testing::Test {
 protected:
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

    settings_ = std::make_shared<olp::client::OlpClientSettings>();
    settings_->network_request_handler = network;
    settings_->authentication_settings = auth_client_settings;

    // setup proxy
    settings_->proxy_settings =
        olp::http::NetworkProxySettings()
            .WithHostname(kMockServerHost)
            .WithPort(kMockServerPort)
            .WithType(olp::http::NetworkProxySettings::Type::HTTP);
    SetUpMockServer(network);
  }

  void TearDown() override {
    auto network = std::move(settings_->network_request_handler);
    settings_.reset();
    mock_server_client_.reset();
  }

  void SetUpMockServer(std::shared_ptr<olp::http::Network> network) {
    // create client to set mock server expectations
    olp::client::OlpClientSettings olp_client_settings;
    olp_client_settings.network_request_handler = network;
    mock_server_client_ = std::make_shared<mockserver::MockServerHelper>(
        olp_client_settings, kTestHrn);
  }

  std::shared_ptr<olp::client::OlpClientSettings> settings_;
  std::shared_ptr<mockserver::MockServerHelper> mock_server_client_;
};

TEST_F(VersionedLayerClientTest, GetPartitions) {
  olp::client::HRN hrn(kTestHrn);
  {
    mock_server_client_->MockAuth();
    mock_server_client_->MockTimestamp();
    mock_server_client_->MockLookupResourceApiResponse(
        mockserver::DefaultResponses::GenerateResourceApisResponse(kTestHrn));
    mock_server_client_->MockGetVersionResponse(
        mockserver::DefaultResponses::GenerateVersionResponse(44));
    mock_server_client_->MockGetPartitionsResponse(
        mockserver::DefaultResponses::GeneratePartitionsResponse(4));
  }

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();

  EXPECT_SUCCESS(partitions_response);
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
  EXPECT_TRUE(mock_server_client_->Verify());
}

TEST_F(VersionedLayerClientTest, GetPartitionsError) {
  olp::client::HRN hrn(kTestHrn);
  {
    mock_server_client_->MockAuth();
    mock_server_client_->MockTimestamp();
    mock_server_client_->MockLookupResourceApiResponse(
        mockserver::DefaultResponses::GenerateResourceApisResponse(kTestHrn));
    mock_server_client_->MockGetVersionResponse(
        mockserver::DefaultResponses::GenerateVersionResponse(44));
    mock_server_client_->MockGetPartitionsError(
        {olp::http::HttpStatusCode::BAD_REQUEST, "Bad request"});
  }

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            partitions_response.GetError().GetErrorCode());
  ASSERT_EQ("Bad request", partitions_response.GetError().GetMessage());
  EXPECT_TRUE(mock_server_client_->Verify());
}

}  // namespace
