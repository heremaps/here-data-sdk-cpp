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
#include "generated/api/QueryApi.h"

namespace {
constexpr auto kLayerid = "test-layer";
constexpr auto kQuadKey = "23618401";
constexpr auto kNodeBaseUrl =
    "https://some.node.base.url/stream/v2/catalogs/"
    "hrn:here:data::olp-here-test:hereos-internal-test-v2";
constexpr auto kUrlQuadTreeIndexVolatile =
    R"(https://some.node.base.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/test-layer/quadkeys/23618401/depths/2)";

constexpr auto kUrlQuadTreeIndexVolatileAllInputs =
    R"(https://some.node.base.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/test-layer/quadkeys/23618401/depths/2?additionalFields=checksum%2CdataSize&billingTag=OlpCppSdkTest)";

constexpr auto kHttpResponseEmpty = R"jsonString()jsonString";
constexpr auto kHttpResponseQuadTreeIndexVolatile =
    R"jsonString({ "parentQuads": [ { "additionalMetadata": "string", "checksum": "string", "compressedDataSize": 0, "dataHandle": "675911FF6236B7C7604BF8B105F1BB58", "dataSize": 0, "crc": "c3f276d7", "partition": "73982", "version": 0 } ], "subQuads": [ { "additionalMetadata": "string", "checksum": "291f66029c232400e3403cd6e9cfd36e", "compressedDataSize": 200, "dataHandle": "1b2ca68f-d4a0-4379-8120-cd025640510c", "dataSize": 1024, "crc": "c3f276d7", "subQuadKey": "string", "version": 1 } ] })jsonString";

using namespace ::testing;
using namespace olp;
using namespace olp::client;
using namespace olp::dataservice::read;

class QueryApiTest : public Test {
 protected:
  void SetUp() override {
    network_mock_ = std::make_shared<NetworkMock>();

    settings_ = std::make_shared<OlpClientSettings>();
    settings_->network_request_handler = network_mock_;

    client_ = OlpClientFactory::Create(*settings_);
    client_->SetBaseUrl(kNodeBaseUrl);
  }
  void TearDown() override { network_mock_.reset(); }

 protected:
  std::shared_ptr<OlpClientSettings> settings_;
  std::shared_ptr<OlpClient> client_;
  std::shared_ptr<NetworkMock> network_mock_;
};

TEST_F(QueryApiTest, QuadTreeIndexVolatile) {
  {
    SCOPED_TRACE("Request metadata for tile.");
    EXPECT_CALL(
        *network_mock_,
        Send(AllOf(IsGetRequest(kUrlQuadTreeIndexVolatile)), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            kHttpResponseQuadTreeIndexVolatile));

    int32_t depth = 2;
    auto index_response =
        olp::dataservice::read::QueryApi::QuadTreeIndexVolatile(
            *client_, kLayerid, kQuadKey, depth, boost::none, boost::none,
            olp::client::CancellationContext{});

    ASSERT_TRUE(index_response.IsSuccessful());
    auto result = index_response.GetResult();
    ASSERT_EQ(1u, result.GetSubQuads().size());
    ASSERT_EQ(1u, result.GetParentQuads().size());
  }
  {
    SCOPED_TRACE("Request with billing tag + additional fields.");
    EXPECT_CALL(*network_mock_,
                Send(AllOf(IsGetRequest(kUrlQuadTreeIndexVolatileAllInputs)), _,
                     _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            kHttpResponseQuadTreeIndexVolatile));

    int32_t depth = 2;
    std::string billing_tag = "OlpCppSdkTest";
    std::vector<std::string> additional_fields = {"checksum", "dataSize"};
    auto index_response =
        olp::dataservice::read::QueryApi::QuadTreeIndexVolatile(
            *client_, kLayerid, kQuadKey, depth, additional_fields, billing_tag,
            olp::client::CancellationContext{});

    ASSERT_TRUE(index_response.IsSuccessful());
    auto result = index_response.GetResult();
    ASSERT_EQ(1u, result.GetSubQuads().size());
    ASSERT_EQ(1u, result.GetParentQuads().size());
  }
  // negative tests
  {
    SCOPED_TRACE("Requested quead not found.");
    EXPECT_CALL(
        *network_mock_,
        Send(AllOf(IsGetRequest(kUrlQuadTreeIndexVolatile)), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::NOT_FOUND),
            kHttpResponseEmpty));

    int32_t depth = 2;
    auto index_response =
        olp::dataservice::read::QueryApi::QuadTreeIndexVolatile(
            *client_, kLayerid, kQuadKey, depth, boost::none, boost::none,
            olp::client::CancellationContext{});

    ASSERT_FALSE(index_response.IsSuccessful());
    auto error = index_response.GetError();
    ASSERT_EQ(http::HttpStatusCode::NOT_FOUND, error.GetHttpStatusCode());
    ASSERT_EQ(kHttpResponseEmpty, error.GetMessage());
  }
  {
    SCOPED_TRACE("Not configured olp client");
    auto client = OlpClient();
    int32_t depth = 2;
    auto index_response =
        olp::dataservice::read::QueryApi::QuadTreeIndexVolatile(
            client, kLayerid, kQuadKey, depth, boost::none, boost::none,
            olp::client::CancellationContext{});

    ASSERT_FALSE(index_response.IsSuccessful());
    auto error = index_response.GetError();
    ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::OFFLINE_ERROR),
              error.GetHttpStatusCode());
  }
  {
    SCOPED_TRACE("Invalid layer id");
    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(http::NetworkResponse().WithStatus(
                                         http::HttpStatusCode::BAD_REQUEST),
                                     kHttpResponseEmpty));
    int32_t depth = 2;
    auto index_response =
        olp::dataservice::read::QueryApi::QuadTreeIndexVolatile(
            *client_, {}, kQuadKey, depth, boost::none, boost::none,
            olp::client::CancellationContext{});

    ASSERT_FALSE(index_response.IsSuccessful());
    auto error = index_response.GetError();
    ASSERT_EQ(http::HttpStatusCode::BAD_REQUEST, error.GetHttpStatusCode());
    ASSERT_EQ(kHttpResponseEmpty, error.GetMessage());
  }
  {
    SCOPED_TRACE("Invalid quad key.");
    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(http::NetworkResponse().WithStatus(
                                         http::HttpStatusCode::BAD_REQUEST),
                                     kHttpResponseEmpty));
    int32_t depth = 2;
    auto index_response =
        olp::dataservice::read::QueryApi::QuadTreeIndexVolatile(
            *client_, kLayerid, {}, depth, boost::none, boost::none,
            olp::client::CancellationContext{});

    ASSERT_FALSE(index_response.IsSuccessful());
    auto error = index_response.GetError();
    ASSERT_EQ(http::HttpStatusCode::BAD_REQUEST, error.GetHttpStatusCode());
    ASSERT_EQ(kHttpResponseEmpty, error.GetMessage());
  }
  {
    SCOPED_TRACE("Invalid depth.");
    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(http::NetworkResponse().WithStatus(
                                         http::HttpStatusCode::BAD_REQUEST),
                                     kHttpResponseEmpty));
    int32_t depth = -1;
    auto index_response =
        olp::dataservice::read::QueryApi::QuadTreeIndexVolatile(
            *client_, kLayerid, kQuadKey, depth, boost::none, boost::none,
            olp::client::CancellationContext{});

    ASSERT_FALSE(index_response.IsSuccessful());
    auto error = index_response.GetError();
    ASSERT_EQ(http::HttpStatusCode::BAD_REQUEST, error.GetHttpStatusCode());
    ASSERT_EQ(kHttpResponseEmpty, error.GetMessage());
  }
  {
    SCOPED_TRACE("Empty additional fields.");
    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(http::NetworkResponse().WithStatus(
                                         http::HttpStatusCode::BAD_REQUEST),
                                     kHttpResponseEmpty));

    int32_t depth = 2;
    std::vector<std::string> empty_fields;
    auto index_response =
        olp::dataservice::read::QueryApi::QuadTreeIndexVolatile(
            *client_, kLayerid, kQuadKey, depth, empty_fields, boost::none,
            olp::client::CancellationContext{});

    ASSERT_FALSE(index_response.IsSuccessful());
    auto error = index_response.GetError();
    ASSERT_EQ(http::HttpStatusCode::BAD_REQUEST, error.GetHttpStatusCode());
    ASSERT_EQ(kHttpResponseEmpty, error.GetMessage());
  }
  {
    SCOPED_TRACE("Empty billing tag.");
    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
        .WillOnce(ReturnHttpResponse(http::NetworkResponse().WithStatus(
                                         http::HttpStatusCode::BAD_REQUEST),
                                     kHttpResponseEmpty));

    int32_t depth = 2;
    std::string empty_tag;
    auto index_response =
        olp::dataservice::read::QueryApi::QuadTreeIndexVolatile(
            *client_, kLayerid, kQuadKey, depth, boost::none, empty_tag,
            olp::client::CancellationContext{});

    ASSERT_FALSE(index_response.IsSuccessful());
    auto error = index_response.GetError();
    ASSERT_EQ(http::HttpStatusCode::BAD_REQUEST, error.GetHttpStatusCode());
    ASSERT_EQ(kHttpResponseEmpty, error.GetMessage());
  }
  {
    SCOPED_TRACE("Cancelled CancellationContext");

    auto context = olp::client::CancellationContext();
    context.CancelOperation();
    int32_t depth = 2;
    auto index_response =
        olp::dataservice::read::QueryApi::QuadTreeIndexVolatile(
            *client_, kLayerid, kQuadKey, depth, boost::none, boost::none,
            context);

    ASSERT_FALSE(index_response.IsSuccessful());
    auto error = index_response.GetError();
    ASSERT_EQ(olp::client::ErrorCode::Cancelled, error.GetErrorCode());
  }
}

}  // namespace
