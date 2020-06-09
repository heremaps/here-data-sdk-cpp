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

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/NetworkMock.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include "generated/api/MetadataApi.h"

namespace {

constexpr auto kStartVersion = 299;
constexpr auto kEndVersion = 300;
constexpr auto kNodeBaseUrl =
    "https://some.node.base.url/metadata/v1/catalogs/"
    "hrn:here:data::olp-here-test:hereos-internal-test-v2";

constexpr auto kUrlVersionsList =
    R"(https://some.node.base.url/metadata/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/versions?endVersion=300&startVersion=299)";

constexpr auto kUrlVersionsListBillingTag =
    R"(https://some.node.base.url/metadata/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/versions?billingTag=OlpCppSdkTest&endVersion=300&startVersion=299)";

constexpr auto kHttpResponse =
    R"jsonString({"versions":[{"version":4,"timestamp":1547159598712,"partitionCounts":{"testlayer":5,"testlayer_res":1,"multilevel_testlayer":33, "hype-test-prefetch-2":7,"testlayer_gzip":1,"hype-test-prefetch":7},"dependencies":[ { "hrn":"hrn:here:data::olp-here-test:hereos-internal-test-v2","version":0,"direct":false},{"hrn":"hrn:here:data:::hereos-internal-test-v2","version":0,"direct":false }]}]})jsonString";

using ::testing::_;
namespace http = olp::http;
namespace client = olp::client;
namespace common = olp::tests::common;

class MetadataApiTest : public testing::Test {
 protected:
  void SetUp() override {
    network_mock_ = std::make_shared<common::NetworkMock>();

    settings_ = std::make_shared<client::OlpClientSettings>();
    settings_->network_request_handler = network_mock_;

    client_ = client::OlpClientFactory::Create(*settings_);
    client_->SetBaseUrl(kNodeBaseUrl);
  }
  void TearDown() override { network_mock_.reset(); }

 protected:
  std::shared_ptr<client::OlpClientSettings> settings_;
  std::shared_ptr<client::OlpClient> client_;
  std::shared_ptr<common::NetworkMock> network_mock_;
};

TEST_F(MetadataApiTest, GetListVersions) {
  {
    SCOPED_TRACE("Request metadata versions.");
    EXPECT_CALL(*network_mock_,
                Send(testing::AllOf(common::IsGetRequest(kUrlVersionsList)), _,
                     _, _, _))
        .WillOnce(common::ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            kHttpResponse));

    auto index_response = olp::dataservice::read::MetadataApi::ListVersions(
        *client_, kStartVersion, kEndVersion, boost::none,
        olp::client::CancellationContext{});

    ASSERT_TRUE(index_response.IsSuccessful());
    auto result = index_response.GetResult();

    ASSERT_EQ(1u, result.GetVersions().size());
    ASSERT_EQ(4, result.GetVersions().front().GetVersion());
    ASSERT_EQ(2u, result.GetVersions().front().GetDependencies().size());
    ASSERT_EQ(6u, result.GetVersions().front().GetPartitionCounts().size());
  }

  {
    SCOPED_TRACE("Request with billing tag.");
    std::string billing_tag = "OlpCppSdkTest";
    EXPECT_CALL(
        *network_mock_,
        Send(testing::AllOf(common::IsGetRequest(kUrlVersionsListBillingTag)),
             _, _, _, _))
        .WillOnce(common::ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            kHttpResponse));

    auto response = olp::dataservice::read::MetadataApi::ListVersions(
        *client_, kStartVersion, kEndVersion, billing_tag,
        olp::client::CancellationContext{});

    ASSERT_TRUE(response.IsSuccessful());
    auto result = response.GetResult();

    ASSERT_EQ(1u, result.GetVersions().size());
    ASSERT_EQ(4, result.GetVersions().front().GetVersion());
    ASSERT_EQ(2u, result.GetVersions().front().GetDependencies().size());
    ASSERT_EQ(6u, result.GetVersions().front().GetPartitionCounts().size());
  }
  {
    SCOPED_TRACE("Cancelled CancellationContext");

    auto context = olp::client::CancellationContext();
    context.CancelOperation();

    auto index_response = olp::dataservice::read::MetadataApi::ListVersions(
        *client_, kStartVersion, kEndVersion, boost::none, context);

    ASSERT_FALSE(index_response.IsSuccessful());
    auto error = index_response.GetError();
    ASSERT_EQ(olp::client::ErrorCode::Cancelled, error.GetErrorCode());
  }
}

}  // namespace
