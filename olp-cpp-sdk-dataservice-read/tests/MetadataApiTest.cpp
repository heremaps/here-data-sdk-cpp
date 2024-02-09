/*
 * Copyright (C) 2020-2024 HERE Europe B.V.
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
#include <olp/core/utils/Url.h>
#include <boost/optional/optional_io.hpp>
#include "TypesToStream.h"
#include "generated/api/MetadataApi.h"

namespace {
constexpr auto kStartVersion = 3;
constexpr auto kEndVersion = 4;
constexpr auto kNodeBaseUrl =
    "https://some.node.base.url/metadata/v1/catalogs/"
    "hrn:here:data::olp-here-test:hereos-internal-test-v2";

constexpr auto kUrlVersionsList =
    R"(https://some.node.base.url/metadata/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/versions?endVersion=4&startVersion=3)";

constexpr auto kUrlVersionsListBillingTag =
    R"(https://some.node.base.url/metadata/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/versions?billingTag=OlpCppSdkTest&endVersion=4&startVersion=3)";

constexpr auto kHttpVersionsListResponse =
    R"jsonString({"versions":[{"version":4,"timestamp":1547159598712,"partitionCounts":{"testlayer":5,"testlayer_res":1,"multilevel_testlayer":33, "hype-test-prefetch-2":7,"testlayer_gzip":1,"hype-test-prefetch":7},"dependencies":[ { "hrn":"hrn:here:data::olp-here-test:hereos-internal-test-v2","version":0,"direct":false},{"hrn":"hrn:here:data:::hereos-internal-test-v2","version":0,"direct":false }]}]})jsonString";

using ::testing::_;
namespace http = olp::http;
namespace client = olp::client;

class MetadataApiTest : public testing::Test {
 protected:
  void SetUp() override {
    network_mock_ = std::make_shared<NetworkMock>();

    settings_ = std::make_shared<client::OlpClientSettings>();
    settings_->network_request_handler = network_mock_;

    client_ = client::OlpClientFactory::Create(*settings_);
    client_->SetBaseUrl(kNodeBaseUrl);
  }
  void TearDown() override { network_mock_.reset(); }

 protected:
  std::shared_ptr<client::OlpClientSettings> settings_;
  std::shared_ptr<client::OlpClient> client_;
  std::shared_ptr<NetworkMock> network_mock_;
};

TEST_F(MetadataApiTest, GetListVersions) {
  {
    SCOPED_TRACE("Request metadata versions.");
    EXPECT_CALL(
        *network_mock_,
        Send(testing::AllOf(IsGetRequest(kUrlVersionsList)), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            kHttpVersionsListResponse));

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
    EXPECT_CALL(*network_mock_,
                Send(testing::AllOf(IsGetRequest(kUrlVersionsListBillingTag)),
                     _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            kHttpVersionsListResponse));

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

struct TestParameters {
  boost::optional<int64_t> version;
  std::string layer;
  std::string url;
  std::vector<std::string> additional_fields;
  boost::optional<std::string> billing_tag;
  boost::optional<std::string> range;
};

class MetadataApiParamTest
    : public MetadataApiTest,
      public testing::WithParamInterface<TestParameters> {};

TEST_P(MetadataApiParamTest, GetPartitionsStream) {
  using testing::AllOf;
  using testing::Eq;
  using testing::Mock;
  using testing::Return;

  const TestParameters test_params = GetParam();

  olp::client::CancellationContext context;

  boost::optional<std::uint64_t> received_offset;
  std::string received_stream_data;

  auto data_callback = [&](const std::uint8_t* data, std::uint64_t offset,
                           std::size_t length) mutable {
    received_stream_data.assign(reinterpret_cast<const char*>(data), length);
    received_offset = offset;
  };

  const std::string ref_stream_data{"reference stream data"};
  const std::uint64_t offset{7};

  const auto range_header = [&]() -> boost::optional<olp::http::Header> {
    if (test_params.range) {
      return olp::http::Header{"Range", test_params.range.value()};
    }
    return boost::none;
  }();

  EXPECT_CALL(*network_mock_, Send(AllOf(IsGetRequest(test_params.url),
                                         HeadersContainOptional(range_header)),
                                   _, _, _, _))
      .WillOnce(ReturnHttpResponseWithDataCallback(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          ref_stream_data, offset));

  client::HttpResponse response =
      olp::dataservice::read::MetadataApi::GetPartitionsStream(
          *client_, test_params.layer, test_params.version,
          test_params.additional_fields, test_params.range,
          test_params.billing_tag, data_callback, context);

  EXPECT_EQ(response.GetStatus(), http::HttpStatusCode::OK);
  EXPECT_STREQ(ref_stream_data.c_str(), received_stream_data.c_str());
  EXPECT_TRUE(received_offset);
  EXPECT_EQ(received_offset.value_or(0), offset);
}

std::vector<TestParameters> GetPartitionsStreamParams() {
  std::vector<TestParameters> config;
  TestParameters params;

  params.billing_tag = boost::none;
  params.layer = "testLayer";
  params.range = boost::none;
  params.version = boost::none;
  params.url = std::string(kNodeBaseUrl) + R"(/layers/)" + params.layer +
               R"(/partitions)";
  config.emplace_back(params);

  params.billing_tag = boost::none;
  params.layer = "testLayer";
  params.range = boost::none;
  params.version = kStartVersion;
  params.url = std::string(kNodeBaseUrl) + R"(/layers/)" + params.layer +
               R"(/partitions?version=)" +
               std::to_string(params.version.value());
  config.emplace_back(params);

  params.billing_tag = boost::none;
  params.layer = "testLayer";
  params.range = "rangeReferenceValue";
  params.version = boost::none;
  params.url = std::string(kNodeBaseUrl) + R"(/layers/)" + params.layer +
               R"(/partitions)";
  config.emplace_back(params);

  params.billing_tag = "billingTagValue";
  params.layer = "testLayer";
  params.range = "rangeReferenceValue";
  params.version = boost::none;
  params.url = std::string(kNodeBaseUrl) + R"(/layers/)" + params.layer +
               R"(/partitions?billingTag=)" + params.billing_tag.value();
  config.emplace_back(params);

  params.additional_fields = {"checksum", "compressedDataSize"};
  params.billing_tag = boost::none;
  params.layer = "testLayer";
  params.range = boost::none;
  params.version = boost::none;
  params.url = std::string(kNodeBaseUrl) + R"(/layers/)" + params.layer +
               R"(/partitions?additionalFields=)" +
               olp::utils::Url::Encode(R"(checksum,compressedDataSize)");
  config.emplace_back(params);

  params.additional_fields = {"compressedDataSize"};
  params.billing_tag = boost::none;
  params.layer = "testLayer";
  params.range = boost::none;
  params.version = kEndVersion;
  params.url = std::string(kNodeBaseUrl) + R"(/layers/)" + params.layer +
               R"(/partitions?additionalFields=)" +
               olp::utils::Url::Encode(R"(compressedDataSize)") +
               R"(&version=)" + std::to_string(params.version.value());
  config.emplace_back(params);

  return config;
}

INSTANTIATE_TEST_SUITE_P(, MetadataApiParamTest,
                         testing::ValuesIn(GetPartitionsStreamParams()));

}  // namespace
