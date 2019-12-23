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

#include <gtest/gtest.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/write/VolatileLayerClient.h>
#include <olp/dataservice/write/model/PublishPartitionDataRequest.h>
#include <testutils/CustomParameters.hpp>
#include "Utils.h"

namespace {

using namespace olp::dataservice::write;
using namespace olp::dataservice::write::model;

const std::string kEndpoint = "endpoint";
const std::string kAppid = "dataservice_write_test_appid";
const std::string kSecret = "dataservice_write_test_secret";
const std::string kCatalog = "dataservice_write_test_catalog";
const std::string kVolatileLayer = "volatile_layer";

// The limit for 100 retries is 10 minutes. Therefore, the wait time between
// retries is 6 seconds.
const auto kWaitBeforeRetry = std::chrono::seconds(6);

void PublishDataSuccessAssertions(
    const olp::client::ApiResponse<
        olp::dataservice::write::model::ResponseOkSingle,
        olp::client::ApiError>& result) {
  EXPECT_SUCCESS(result);
  EXPECT_FALSE(result.GetResult().GetTraceID().empty());
  EXPECT_EQ("", result.GetError().GetMessage());
}

class DataserviceWriteVolatileLayerClientTest : public ::testing::Test {
 protected:
  virtual void SetUp() override {
    client_ = CreateVolatileLayerClient();
    data_ = GenerateData();
  }

  virtual void TearDown() override {
    data_.reset();
    client_.reset();
  }

  std::string GetTestCatalog() {
    return CustomParameters::getArgument(kCatalog);
  }

  std::string GetTestLayer() {
    return CustomParameters::getArgument(kVolatileLayer);
  }
  static void SetUpTestSuite() {
    s_network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();
  }

  virtual std::shared_ptr<VolatileLayerClient> CreateVolatileLayerClient() {
    auto network = s_network;

    const auto key_id = CustomParameters::getArgument(kAppid);
    const auto secret = CustomParameters::getArgument(kSecret);

    olp::authentication::Settings authentication_settings({key_id, secret});
    authentication_settings.token_endpoint_url =
        CustomParameters::getArgument(kEndpoint);
    authentication_settings.network_request_handler = network;

    olp::authentication::TokenProviderDefault provider(authentication_settings);

    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;

    olp::client::OlpClientSettings settings;
    settings.authentication_settings = auth_client_settings;
    settings.network_request_handler = network;

    return std::make_shared<VolatileLayerClient>(
        olp::client::HRN{GetTestCatalog()}, settings);
  }

 private:
  std::shared_ptr<std::vector<unsigned char>> GenerateData() {
    std::string test_suite_name(testing::UnitTest::GetInstance()
                                    ->current_test_info()
                                    ->test_suite_name());
    std::string test_name(
        testing::UnitTest::GetInstance()->current_test_info()->name());
    std::string data_string(test_suite_name + " " + test_name + " Payload");
    return std::make_shared<std::vector<unsigned char>>(data_string.begin(),
                                                        data_string.end());
  }

 protected:
  static std::shared_ptr<olp::http::Network> s_network;

  std::shared_ptr<VolatileLayerClient> client_;
  std::shared_ptr<std::vector<unsigned char>> data_;
};

// Static network instance is necessary as it needs to outlive any created
// clients. This is a known limitation as triggered send requests capture the
// network instance inside the callbacks.
std::shared_ptr<olp::http::Network>
    DataserviceWriteVolatileLayerClientTest::s_network;

TEST_F(DataserviceWriteVolatileLayerClientTest, GetBaseVersion) {
  auto volatile_client = CreateVolatileLayerClient();
  auto response = volatile_client->GetBaseVersion().GetFuture().get();

  EXPECT_SUCCESS(response);
  auto version_response = response.MoveResult();
  ASSERT_GE(version_response.GetVersion(), 0);
}

TEST_F(DataserviceWriteVolatileLayerClientTest, StartBatchInvalid) {
  auto volatile_client = CreateVolatileLayerClient();
  auto response =
      volatile_client->StartBatch(StartBatchRequest()).GetFuture().get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_FALSE(response.GetResult().GetId());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            response.GetError().GetErrorCode());

  auto get_batch_response =
      volatile_client->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_FALSE(get_batch_response.IsSuccessful());

  auto complete_batch_response =
      volatile_client->CompleteBatch(get_batch_response.GetResult())
          .GetFuture()
          .get();
  ASSERT_FALSE(complete_batch_response.IsSuccessful());
}

TEST_F(DataserviceWriteVolatileLayerClientTest, StartBatch) {
  auto volatile_client = CreateVolatileLayerClient();
  auto response =
      volatile_client
          ->StartBatch(StartBatchRequest().WithLayers({GetTestLayer()}))
          .GetFuture()
          .get();

  EXPECT_SUCCESS(response);
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto get_batch_response =
      volatile_client->GetBatch(response.GetResult()).GetFuture().get();

  EXPECT_SUCCESS(get_batch_response);
  ASSERT_EQ(response.GetResult().GetId().value(),
            get_batch_response.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            get_batch_response.GetResult().GetDetails()->GetState());

  auto complete_batch_response =
      volatile_client->CompleteBatch(get_batch_response.GetResult())
          .GetFuture()
          .get();
  EXPECT_SUCCESS(complete_batch_response);

  for (int i = 0; i < 100; ++i) {
    get_batch_response =
        volatile_client->GetBatch(response.GetResult()).GetFuture().get();

    EXPECT_SUCCESS(get_batch_response);
    ASSERT_EQ(response.GetResult().GetId().value(),
              get_batch_response.GetResult().GetId().value());
    if (get_batch_response.GetResult().GetDetails()->GetState() !=
        "succeeded") {
      ASSERT_EQ("submitted",
                get_batch_response.GetResult().GetDetails()->GetState());
      std::this_thread::sleep_for(kWaitBeforeRetry);
    } else {
      break;
    }
  }

  ASSERT_EQ("succeeded",
            get_batch_response.GetResult().GetDetails()->GetState());
}

TEST_F(DataserviceWriteVolatileLayerClientTest, PublishToBatch) {
  auto volatile_client = CreateVolatileLayerClient();
  auto response =
      volatile_client
          ->StartBatch(StartBatchRequest().WithLayers({GetTestLayer()}))
          .GetFuture()
          .get();

  EXPECT_SUCCESS(response);
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  std::vector<PublishPartitionDataRequest> partition_requests;
  PublishPartitionDataRequest partition_request;
  partition_requests.push_back(
      partition_request.WithLayerId(GetTestLayer()).WithPartitionId("123"));
  partition_requests.push_back(partition_request.WithPartitionId("456"));

  auto publish_to_batch_response =
      volatile_client->PublishToBatch(response.GetResult(), partition_requests)
          .GetFuture()
          .get();
  EXPECT_SUCCESS(publish_to_batch_response);

  auto complete_batch_response =
      volatile_client->CompleteBatch(response.GetResult()).GetFuture().get();
  EXPECT_SUCCESS(complete_batch_response);

  GetBatchResponse get_batch_response;
  for (int i = 0; i < 100; ++i) {
    get_batch_response =
        volatile_client->GetBatch(response.GetResult()).GetFuture().get();

    EXPECT_SUCCESS(get_batch_response);
    ASSERT_EQ(response.GetResult().GetId().value(),
              get_batch_response.GetResult().GetId().value());
    if (get_batch_response.GetResult().GetDetails()->GetState() !=
        "succeeded") {
      ASSERT_EQ("submitted",
                get_batch_response.GetResult().GetDetails()->GetState());
      std::this_thread::sleep_for(kWaitBeforeRetry);
    } else {
      break;
    }
  }

  ASSERT_EQ("succeeded",
            get_batch_response.GetResult().GetDetails()->GetState());
}

TEST_F(DataserviceWriteVolatileLayerClientTest, PublishToBatchInvalid) {
  auto volatile_client = CreateVolatileLayerClient();
  auto response =
      volatile_client
          ->StartBatch(StartBatchRequest().WithLayers({GetTestLayer()}))
          .GetFuture()
          .get();

  EXPECT_SUCCESS(response);
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto publish_to_batch_response =
      volatile_client->PublishToBatch(response.GetResult(), {})
          .GetFuture()
          .get();
  ASSERT_FALSE(publish_to_batch_response.IsSuccessful());

  std::vector<PublishPartitionDataRequest> partition_requests{
      PublishPartitionDataRequest{}, PublishPartitionDataRequest{}};

  publish_to_batch_response =
      volatile_client->PublishToBatch(response.GetResult(), partition_requests)
          .GetFuture()
          .get();
  ASSERT_FALSE(publish_to_batch_response.IsSuccessful());

  partition_requests.clear();
  PublishPartitionDataRequest partition_request;
  partition_requests.push_back(
      partition_request.WithLayerId("foo").WithPartitionId("123"));
  partition_requests.push_back(
      partition_request.WithLayerId("bar").WithPartitionId("456"));

  publish_to_batch_response =
      volatile_client->PublishToBatch(response.GetResult(), partition_requests)
          .GetFuture()
          .get();
  ASSERT_FALSE(publish_to_batch_response.IsSuccessful());
}

// Sometimes we receive the 500 internal server error,
// thus looks loke the problem is on the server side.
// Please, re-enable this test when switched to mocked server or
// when the server will be more steady for testing
TEST_F(DataserviceWriteVolatileLayerClientTest,
       DISABLED_StartBatchDeleteClient) {
  auto volatile_client = CreateVolatileLayerClient();
  auto response =
      volatile_client
          ->StartBatch(StartBatchRequest().WithLayers({GetTestLayer()}))
          .GetFuture()
          .get();

  EXPECT_SUCCESS(response);
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto get_batch_future =
      volatile_client->GetBatch(response.GetResult()).GetFuture();

  volatile_client = nullptr;

  auto get_batch_response = get_batch_future.get();
  EXPECT_SUCCESS(get_batch_response);
  ASSERT_EQ(response.GetResult().GetId().value(),
            get_batch_response.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            get_batch_response.GetResult().GetDetails()->GetState());

  volatile_client = CreateVolatileLayerClient();

  auto complete_batch_response =
      volatile_client->CompleteBatch(get_batch_response.GetResult())
          .GetFuture()
          .get();
  EXPECT_SUCCESS(complete_batch_response);

  for (int i = 0; i < 100; ++i) {
    get_batch_response =
        volatile_client->GetBatch(response.GetResult()).GetFuture().get();

    EXPECT_SUCCESS(get_batch_response);
    ASSERT_EQ(response.GetResult().GetId().value(),
              get_batch_response.GetResult().GetId().value());
    if (get_batch_response.GetResult().GetDetails()->GetState() !=
        "succeeded") {
      ASSERT_EQ("submitted",
                get_batch_response.GetResult().GetDetails()->GetState());
      std::this_thread::sleep_for(kWaitBeforeRetry);
    } else {
      break;
    }
  }

  ASSERT_EQ("succeeded",
            get_batch_response.GetResult().GetDetails()->GetState());
}

TEST_F(DataserviceWriteVolatileLayerClientTest, CancelAllRequests) {
  auto volatile_client = CreateVolatileLayerClient();
  auto future = volatile_client->GetBaseVersion().GetFuture();

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  volatile_client->CancelAll();

  auto response = future.get();
  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_F(DataserviceWriteVolatileLayerClientTest, PublishData) {
  auto response = client_
                      ->PublishPartitionData(PublishPartitionDataRequest()
                                                 .WithData(data_)
                                                 .WithLayerId(GetTestLayer())
                                                 .WithPartitionId("123"))
                      .GetFuture()
                      .get();
  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_F(DataserviceWriteVolatileLayerClientTest, PublishDataAsync) {
  std::promise<PublishPartitionDataResponse> response_promise;
  bool call_is_async = true;

  auto cancel_token = client_->PublishPartitionData(
      PublishPartitionDataRequest()
          .WithData(data_)
          .WithLayerId(GetTestLayer())
          .WithPartitionId("456"),
      [&](const PublishPartitionDataResponse& response) {
        call_is_async = false;
        response_promise.set_value(response);
      });

  EXPECT_TRUE(call_is_async);
  auto response_future = response_promise.get_future();
  auto status = response_future.wait_for(std::chrono::seconds(30));
  if (status != std::future_status::ready) {
    cancel_token.Cancel();
  }
  auto response = response_future.get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

}  // namespace
