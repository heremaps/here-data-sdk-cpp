/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/write/IndexLayerClient.h>
#include <olp/dataservice/write/model/PublishIndexRequest.h>
#include <testutils/CustomParameters.hpp>
#include "Utils.h"

namespace {

namespace write = olp::dataservice::write;
namespace model = olp::dataservice::write::model;

const std::string kEndpoint = "endpoint";
const std::string kAppid = "dataservice_write_test_appid";
const std::string kSecret = "dataservice_write_test_secret";
const std::string kCatalog = "dataservice_write_test_catalog";
const std::string kIndexLayer = "index_layer";

void PublishDataSuccessAssertions(
    const olp::client::ApiResponse<
        olp::dataservice::write::model::ResponseOkSingle,
        olp::client::ApiError>& result) {
  EXPECT_SUCCESS(result);
  EXPECT_FALSE(result.GetResult().GetTraceID().empty());
  EXPECT_EQ("", result.GetError().GetMessage());
}

template <typename T>
void PublishFailureAssertions(
    const olp::client::ApiResponse<T, olp::client::ApiError>& result) {
  EXPECT_FALSE(result.IsSuccessful());
  EXPECT_NE(result.GetError().GetHttpStatusCode(),
            olp::http::HttpStatusCode::OK);
  EXPECT_FALSE(result.GetError().GetMessage().empty());
}

class DataserviceWriteIndexLayerClientTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    s_network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();
  }

  std::shared_ptr<write::IndexLayerClient> CreateIndexLayerClient() {
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
    settings.task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();

    return std::make_shared<write::IndexLayerClient>(
        olp::client::HRN{GetTestCatalog()}, settings);
  }

  void SetUp() override {
    client_ = CreateIndexLayerClient();
    data_ = GenerateData();
  }

  void TearDown() override {
    data_.reset();
    client_.reset();
  }

  std::string GetTestCatalog() {
    return CustomParameters::getArgument(kCatalog);
  }

  std::string GetTestLayer() {
    return CustomParameters::getArgument(kIndexLayer);
  }

  const model::Index GetTestIndex() {
    model::Index index;
    std::map<model::IndexName, std::shared_ptr<model::IndexValue>> index_fields;
    index_fields.insert(
        std::pair<model::IndexName, std::shared_ptr<model::IndexValue>>(
            "Place", std::make_shared<model::StringIndexValue>(
                         "New York", model::IndexType::String)));
    index_fields.insert(
        std::pair<model::IndexName, std::shared_ptr<model::IndexValue>>(
            "Temperature",
            std::make_shared<model::IntIndexValue>(10, model::IndexType::Int)));
    index_fields.insert(
        std::pair<model::IndexName, std::shared_ptr<model::IndexValue>>(
            "Rain", std::make_shared<model::BooleanIndexValue>(
                        false, model::IndexType::Bool)));
    index_fields.insert(
        std::pair<model::IndexName, std::shared_ptr<model::IndexValue>>(
            "testIndexLayer", std::make_shared<model::TimeWindowIndexValue>(
                                  123123, model::IndexType::TimeWindow)));

    index.SetIndexFields(index_fields);
    return index;
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

  std::shared_ptr<write::IndexLayerClient> client_;
  std::shared_ptr<std::vector<unsigned char>> data_;
};

// Static network instance is necessary as it needs to outlive any created
// clients. This is a known limitation as triggered send requests capture the
// network instance inside the callbacks.
std::shared_ptr<olp::http::Network>
    DataserviceWriteIndexLayerClientTest::s_network;

TEST_F(DataserviceWriteIndexLayerClientTest, PublishData) {
  auto response = client_
                      ->PublishIndex(model::PublishIndexRequest()
                                         .WithIndex(GetTestIndex())
                                         .WithData(data_)
                                         .WithLayerId(GetTestLayer()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_F(DataserviceWriteIndexLayerClientTest, DeleteData) {
  auto response = client_
                      ->PublishIndex(model::PublishIndexRequest()
                                         .WithIndex(GetTestIndex())
                                         .WithData(data_)
                                         .WithLayerId(GetTestLayer()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
  auto index_id = response.GetResult().GetTraceID();

  auto delete_index_response =
      client_
          ->DeleteIndexData(
              model::DeleteIndexDataRequest().WithIndexId(index_id).WithLayerId(
                  GetTestLayer()))
          .GetFuture()
          .get();

  EXPECT_SUCCESS(delete_index_response);
}

TEST_F(DataserviceWriteIndexLayerClientTest, PublishDataAsync) {
  std::promise<write::PublishIndexResponse> response_promise;
  bool call_is_async = true;

  auto cancel_token =
      client_->PublishIndex(model::PublishIndexRequest()
                                .WithIndex(GetTestIndex())
                                .WithData(data_)
                                .WithLayerId(GetTestLayer()),
                            [&](const write::PublishIndexResponse& response) {
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

TEST_F(DataserviceWriteIndexLayerClientTest, UpdateIndex) {
  const auto id = "2f269191-5ef7-42a4-a445-fdfe53f95d92";

  {
    SCOPED_TRACE("Add index");
    model::Index index = GetTestIndex();
    index.SetId(id);

    auto response_addition = client_
                                 ->UpdateIndex(model::UpdateIndexRequest()
                                                   .WithIndexAdditions({index})
                                                   .WithLayerId(GetTestLayer()))
                                 .GetFuture()
                                 .get();

    EXPECT_SUCCESS(response_addition);
    EXPECT_EQ("", response_addition.GetError().GetMessage());
  }
  {
    SCOPED_TRACE("Remove index");
    auto response_removal =
        client_
            ->UpdateIndex(
                model::UpdateIndexRequest().WithIndexRemovals({id}).WithLayerId(
                    GetTestLayer()))
            .GetFuture()
            .get();

    EXPECT_SUCCESS(response_removal);
    EXPECT_EQ("", response_removal.GetError().GetMessage());
  }
}

TEST_F(DataserviceWriteIndexLayerClientTest, PublishNoData) {
  auto response = client_
                      ->PublishIndex(model::PublishIndexRequest()
                                         .WithIndex(GetTestIndex())
                                         .WithLayerId(GetTestLayer()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            response.GetError().GetErrorCode());
  ASSERT_EQ("Request data empty.", response.GetError().GetMessage());
}

TEST_F(DataserviceWriteIndexLayerClientTest, PublishNoLayer) {
  auto response = client_
                      ->PublishIndex(model::PublishIndexRequest()
                                         .WithIndex(GetTestIndex())
                                         .WithData(data_)
                                         .WithLayerId("invalid-layer"))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            response.GetError().GetErrorCode());
  ASSERT_EQ(
      "Layer 'invalid-layer' not found in catalog "
      "'hrn:here:data::olp-here-test:olp-cpp-sdk-ingestion-test-catalog'",
      response.GetError().GetMessage());
}

}  // namespace
