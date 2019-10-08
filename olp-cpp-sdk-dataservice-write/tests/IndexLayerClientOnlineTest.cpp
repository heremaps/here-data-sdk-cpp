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
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/write/IndexLayerClient.h>
#include <olp/dataservice/write/model/PublishIndexRequest.h>
#include <testutils/CustomParameters.hpp>

namespace {

using namespace olp::dataservice::write;
using namespace olp::dataservice::write::model;

const std::string kEndpoint = "endpoint";
const std::string kAppid = "dataservice_write_test_appid";
const std::string kSecret = "dataservice_write_test_secret";
const std::string kCatalog = "dataservice_write_test_catalog";
const std::string kIndexLayer = "index_layer";

void PublishDataSuccessAssertions(
    const olp::client::ApiResponse<
        olp::dataservice::write::model::ResponseOkSingle,
        olp::client::ApiError>& result) {
  EXPECT_TRUE(result.IsSuccessful());
  EXPECT_FALSE(result.GetResult().GetTraceID().empty());
  EXPECT_EQ("", result.GetError().GetMessage());
}

template <typename T>
void PublishFailureAssertions(
    const olp::client::ApiResponse<T, olp::client::ApiError>& result) {
  EXPECT_FALSE(result.IsSuccessful());
  EXPECT_NE(result.GetError().GetHttpStatusCode(), 200);
  EXPECT_FALSE(result.GetError().GetMessage().empty());
}

class IndexLayerClientOnlineTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    s_network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();
  }

  std::shared_ptr<IndexLayerClient> CreateIndexLayerClient() {
    auto network = s_network;

    olp::authentication::Settings authentication_settings;
    authentication_settings.token_endpoint_url =
        CustomParameters::getArgument(kEndpoint);
    authentication_settings.network_request_handler = network;

    olp::authentication::TokenProviderDefault provider(
        CustomParameters::getArgument(kAppid),
        CustomParameters::getArgument(kSecret), authentication_settings);

    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;

    olp::client::OlpClientSettings settings;
    settings.authentication_settings = auth_client_settings;
    settings.network_request_handler = network;

    return std::make_shared<IndexLayerClient>(
        olp::client::HRN{GetTestCatalog()}, settings);
  }

  virtual void SetUp() override {
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

  const Index GetTestIndex() {
    Index index;
    std::map<IndexName, std::shared_ptr<IndexValue>> index_fields;
    index_fields.insert(std::pair<IndexName, std::shared_ptr<IndexValue>>(
        "Place",
        std::make_shared<StringIndexValue>("New York", IndexType::String)));
    index_fields.insert(std::pair<IndexName, std::shared_ptr<IndexValue>>(
        "Temperature", std::make_shared<IntIndexValue>(10, IndexType::Int)));
    index_fields.insert(std::pair<IndexName, std::shared_ptr<IndexValue>>(
        "Rain", std::make_shared<BooleanIndexValue>(false, IndexType::Bool)));
    index_fields.insert(std::pair<IndexName, std::shared_ptr<IndexValue>>(
        "testIndexLayer",
        std::make_shared<TimeWindowIndexValue>(123123, IndexType::TimeWindow)));

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

  std::shared_ptr<IndexLayerClient> client_;
  std::shared_ptr<std::vector<unsigned char>> data_;
};

// Static network instance is necessary as it needs to outlive any created
// clients. This is a known limitation as triggered send requests capture the
// network instance inside the callbacks.
std::shared_ptr<olp::http::Network> IndexLayerClientOnlineTest::s_network;

TEST_F(IndexLayerClientOnlineTest, PublishData) {
  auto response = client_
                      ->PublishIndex(PublishIndexRequest()
                                         .WithIndex(GetTestIndex())
                                         .WithData(data_)
                                         .WithLayerId(GetTestLayer()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_F(IndexLayerClientOnlineTest, DeleteData) {
  auto response = client_
                      ->PublishIndex(PublishIndexRequest()
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
              DeleteIndexDataRequest().WithIndexId(index_id).WithLayerId(
                  GetTestLayer()))
          .GetFuture()
          .get();

  ASSERT_TRUE(delete_index_response.IsSuccessful());
}

TEST_F(IndexLayerClientOnlineTest, PublishDataAsync) {
  std::promise<PublishIndexResponse> response_promise;
  bool call_is_async = true;

  auto cancel_token =
      client_->PublishIndex(PublishIndexRequest()
                                .WithIndex(GetTestIndex())
                                .WithData(data_)
                                .WithLayerId(GetTestLayer()),
                            [&](const PublishIndexResponse& response) {
                              call_is_async = false;
                              response_promise.set_value(response);
                            });

  EXPECT_TRUE(call_is_async);
  auto response_future = response_promise.get_future();
  auto status = response_future.wait_for(std::chrono::seconds(30));
  if (status != std::future_status::ready) {
    cancel_token.cancel();
  }
  auto response = response_future.get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_F(IndexLayerClientOnlineTest, UpdateIndex) {
  Index index = GetTestIndex();
  index.SetId("2f269191-5ef7-42a4-a445-fdfe53f95d92");

  auto response =
      client_
          ->UpdateIndex(
              UpdateIndexRequest()
                  .WithIndexAdditions({index})
                  .WithIndexRemovals({"2f269191-5ef7-42a4-a445-fdfe53f95d92"})
                  .WithLayerId(GetTestLayer()))
          .GetFuture()
          .get();

  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ("", response.GetError().GetMessage());
}

TEST_F(IndexLayerClientOnlineTest, PublishNoData) {
  auto response = client_
                      ->PublishIndex(PublishIndexRequest()
                                         .WithIndex(GetTestIndex())
                                         .WithLayerId(GetTestLayer()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            response.GetError().GetErrorCode());
  ASSERT_EQ("Request data empty.", response.GetError().GetMessage());
}

TEST_F(IndexLayerClientOnlineTest, PublishNoLayer) {
  auto response = client_
                      ->PublishIndex(PublishIndexRequest()
                                         .WithIndex(GetTestIndex())
                                         .WithData(data_)
                                         .WithLayerId("invalid-layer"))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            response.GetError().GetErrorCode());
  ASSERT_EQ(
      "Unable to find the Layer ID (invalid-layer) "
      "provided in "
      "the PublishIndexRequest in the "
      "Catalog specified when creating "
      "this "
      "IndexLayerClient instance.",
      response.GetError().GetMessage());
}

}  // namespace
