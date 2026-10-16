/*
 * Copyright (C) 2026 HERE Europe B.V.
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

#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/NetworkMock.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include "generated/api/BlobApi.h"

namespace {
using ::testing::_;
using ::testing::AllOf;
using ::testing::Mock;
namespace http = olp::http;
namespace client = olp::client;
namespace read = olp::dataservice::read;
namespace model = olp::dataservice::read::model;

std::string ApiErrorToString(const client::ApiError& error) {
  std::ostringstream result_stream;
  result_stream << "ERROR: code: " << static_cast<int>(error.GetErrorCode())
                << ", status: " << error.GetHttpStatusCode()
                << ", message: " << error.GetMessage();
  return result_stream.str();
}

const std::string kBaseUrl{
    "https://some.blob.base.url/blobstore/v1/catalogs/"
    "hrn:here:data::olp-here-test:hereos-internal-test-v2"};
const std::string kLayerId{"testlayer"};
const std::string kDataHandle{"d5d73b64-7365-41c3-8faf-aa6ad5bab135"};
const std::string kBlobData{"plain-data-blob"};

class BlobApiTest : public testing::Test {
 protected:
  void SetUp() override {
    network_mock_ = std::make_shared<NetworkMock>();

    client::OlpClientSettings settings;
    settings.network_request_handler = network_mock_;
    settings.task_scheduler =
        client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
    olp_client_ = client::OlpClient(settings, kBaseUrl);
  }

  void TearDown() override { network_mock_.reset(); }

 protected:
  client::OlpClient olp_client_;
  std::shared_ptr<NetworkMock> network_mock_;
};

TEST_F(BlobApiTest, GetBlobByKey) {
  {
    SCOPED_TRACE("GetBlobByKey succeeds");

    const auto kUrl = kBaseUrl + "/layers/" + kLayerId + "/keys/test-key";

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            kBlobData));

    client::CancellationContext context;
    const auto response = read::BlobApi::GetBlobByKey(
        olp_client_, kLayerId, "test-key", olp::porting::none,
        olp::porting::none, context);

    ASSERT_TRUE(response.IsSuccessful())
        << ApiErrorToString(response.GetError());
    ASSERT_NE(nullptr, response.GetResult());
    EXPECT_EQ(kBlobData.size(), response.GetResult()->size());
    EXPECT_EQ(kBlobData, std::string(response.GetResult()->begin(),
                                     response.GetResult()->end()));

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetBlobByKey URL-encodes the key");

    const auto kUrl = kBaseUrl + "/layers/" + kLayerId + "/keys/test%2Fkey";

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            kBlobData));

    client::CancellationContext context;
    const auto response = read::BlobApi::GetBlobByKey(
        olp_client_, kLayerId, "test/key", olp::porting::none,
        olp::porting::none, context);

    ASSERT_TRUE(response.IsSuccessful())
        << ApiErrorToString(response.GetError());
    EXPECT_EQ(kBlobData, std::string(response.GetResult()->begin(),
                                     response.GetResult()->end()));

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetBlobByKey passes billing tag and range");

    const auto kUrl =
        kBaseUrl + "/layers/" + kLayerId + "/keys/test-key?billingTag=tag12345";

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            kBlobData));

    client::CancellationContext context;
    const auto response = read::BlobApi::GetBlobByKey(
        olp_client_, kLayerId, "test-key", std::string("tag12345"),
        std::string("bytes=10-"), context);

    ASSERT_TRUE(response.IsSuccessful())
        << ApiErrorToString(response.GetError());
    EXPECT_EQ(kBlobData, std::string(response.GetResult()->begin(),
                                     response.GetResult()->end()));

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetBlobByKey fails with 404");

    const auto kUrl = kBaseUrl + "/layers/" + kLayerId + "/keys/test-key";

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::NOT_FOUND),
            ""));

    client::CancellationContext context;
    const auto response = read::BlobApi::GetBlobByKey(
        olp_client_, kLayerId, "test-key", olp::porting::none,
        olp::porting::none, context);

    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              http::HttpStatusCode::NOT_FOUND);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetBlobByKey fails with 403");

    const auto kUrl = kBaseUrl + "/layers/" + kLayerId + "/keys/test-key";

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::FORBIDDEN),
            "Forbidden"));

    client::CancellationContext context;
    const auto response = read::BlobApi::GetBlobByKey(
        olp_client_, kLayerId, "test-key", olp::porting::none,
        olp::porting::none, context);

    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              http::HttpStatusCode::FORBIDDEN);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetBlobByKey is cancelled before the request is sent");

    client::CancellationContext context;
    context.CancelOperation();
    ASSERT_TRUE(context.IsCancelled());

    const auto response = read::BlobApi::GetBlobByKey(
        olp_client_, kLayerId, "test-key", olp::porting::none,
        olp::porting::none, context);

    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
  }
}

TEST_F(BlobApiTest, GetBlob) {
  {
    SCOPED_TRACE("GetBlob succeeds");

    const auto kUrl = kBaseUrl + "/layers/" + kLayerId + "/data/" + kDataHandle;

    EXPECT_CALL(
        *network_mock_,
        Send(AllOf(IsGetRequest(kUrl),
                   HeadersContain(http::Header("Accept", "application/json"))),
             _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            kBlobData));

    model::Partition partition;
    partition.SetDataHandle(kDataHandle);
    partition.SetDataSize(1024);

    client::CancellationContext context;
    const auto response =
        read::BlobApi::GetBlob(olp_client_, kLayerId, partition,
                               olp::porting::none, olp::porting::none, context);

    ASSERT_TRUE(response.IsSuccessful())
        << ApiErrorToString(response.GetError());
    ASSERT_NE(nullptr, response.GetResult());
    EXPECT_EQ(kBlobData.size(), response.GetResult()->size());
    EXPECT_EQ(kBlobData, std::string(response.GetResult()->begin(),
                                     response.GetResult()->end()));

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetBlob passes billing tag and range");

    const auto kUrl = kBaseUrl + "/layers/" + kLayerId + "/data/" +
                      kDataHandle + "?billingTag=tag12345";

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            kBlobData));

    model::Partition partition;
    partition.SetDataHandle(kDataHandle);

    client::CancellationContext context;
    const auto response = read::BlobApi::GetBlob(
        olp_client_, kLayerId, partition, std::string("tag12345"),
        std::string("bytes=10-"), context);

    ASSERT_TRUE(response.IsSuccessful())
        << ApiErrorToString(response.GetError());
    EXPECT_EQ(kBlobData, std::string(response.GetResult()->begin(),
                                     response.GetResult()->end()));

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetBlob fails with 404");

    const auto kUrl = kBaseUrl + "/layers/" + kLayerId + "/data/" + kDataHandle;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::NOT_FOUND),
            ""));

    model::Partition partition;
    partition.SetDataHandle(kDataHandle);

    client::CancellationContext context;
    const auto response =
        read::BlobApi::GetBlob(olp_client_, kLayerId, partition,
                               olp::porting::none, olp::porting::none, context);

    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              http::HttpStatusCode::NOT_FOUND);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetBlob fails with 403");

    const auto kUrl = kBaseUrl + "/layers/" + kLayerId + "/data/" + kDataHandle;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(kUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::FORBIDDEN),
            "Forbidden"));

    model::Partition partition;
    partition.SetDataHandle(kDataHandle);

    client::CancellationContext context;
    const auto response =
        read::BlobApi::GetBlob(olp_client_, kLayerId, partition,
                               olp::porting::none, olp::porting::none, context);

    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              http::HttpStatusCode::FORBIDDEN);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetBlob is cancelled before the request is sent");

    model::Partition partition;
    partition.SetDataHandle(kDataHandle);

    client::CancellationContext context;
    context.CancelOperation();
    ASSERT_TRUE(context.IsCancelled());

    const auto response =
        read::BlobApi::GetBlob(olp_client_, kLayerId, partition,
                               olp::porting::none, olp::porting::none, context);

    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
  }
}

}  // namespace
