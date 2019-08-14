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

#include <gmock/gmock.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/network/HttpResponse.h>

#include <olp/dataservice/write/IndexLayerClient.h>
#include <olp/dataservice/write/model/PublishIndexRequest.h>

#include "testutils/CustomParameters.hpp"

#include "HttpResponses.h"

using namespace olp::dataservice::write;
using namespace olp::dataservice::write::model;

const std::string kEndpoint = "endpoint";
const std::string kAppid = "appid";
const std::string kSecret = "secret";
const std::string kCatalog = "catalog";
const std::string kIndexLayer = "index_layer";

// TODO: Move duplicate test code to common header
namespace {
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

template <typename T>
void PublishCancelledAssertions(
    const olp::client::ApiResponse<T, olp::client::ApiError>& result) {
  EXPECT_FALSE(result.IsSuccessful());
  EXPECT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(result.GetError().GetHttpStatusCode()));
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            result.GetError().GetErrorCode());
  EXPECT_EQ("Cancelled", result.GetError().GetMessage());
}
}  // namespace

class IndexLayerClientTestBase : public ::testing::TestWithParam<bool> {
 protected:
  virtual void SetUp() override {
    client_ = CreateIndexLayerClient();
    data_ = GenerateData();
  }

  virtual void TearDown() override { client_ = nullptr; }

  virtual bool IsOnlineTest() { return GetParam(); }

  std::string GetTestCatalog() {
    return IsOnlineTest()
               ? CustomParameters::getArgument(kCatalog)
               : "hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog";
  }

  std::string GetTestLayer() {
    return IsOnlineTest() ? CustomParameters::getArgument(kIndexLayer)
                          : "olp-cpp-sdk-ingestion-test-index-layer";
  }

  const Index GetTestIndex() {
    Index index;
    std::map<IndexName, std::shared_ptr<IndexValue>> indexFields;
    indexFields.insert(std::pair<IndexName, std::shared_ptr<IndexValue>>(
        "Place",
        std::make_shared<StringIndexValue>("New York", IndexType::String)));
    indexFields.insert(std::pair<IndexName, std::shared_ptr<IndexValue>>(
        "Temperature", std::make_shared<IntIndexValue>(10, IndexType::Int)));
    indexFields.insert(std::pair<IndexName, std::shared_ptr<IndexValue>>(
        "Rain", std::make_shared<BooleanIndexValue>(false, IndexType::Bool)));
    indexFields.insert(std::pair<IndexName, std::shared_ptr<IndexValue>>(
        "testIndexLayer",
        std::make_shared<TimeWindowIndexValue>(123123, IndexType::TimeWindow)));

    index.SetIndexFields(indexFields);
    return index;
  }

  virtual std::shared_ptr<IndexLayerClient> CreateIndexLayerClient() = 0;

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
  std::shared_ptr<IndexLayerClient> client_;
  std::shared_ptr<std::vector<unsigned char>> data_;
};

class IndexLayerClientOnlineTest : public IndexLayerClientTestBase {
 protected:
  virtual std::shared_ptr<IndexLayerClient> CreateIndexLayerClient() override {
    olp::authentication::Settings settings;
    settings.token_endpoint_url = CustomParameters::getArgument(kEndpoint);

    olp::client::OlpClientSettings client_settings;
    client_settings.authentication_settings =
        (olp::client::AuthenticationSettings{
            olp::authentication::TokenProviderDefault{
                CustomParameters::getArgument(kAppid),
                CustomParameters::getArgument(kSecret), settings}});

    return std::make_shared<IndexLayerClient>(
        olp::client::HRN{GetTestCatalog()}, client_settings);
  }
};

INSTANTIATE_TEST_SUITE_P(TestOnline, IndexLayerClientOnlineTest,
                         ::testing::Values(true));

TEST_P(IndexLayerClientOnlineTest, PublishData) {
  auto response = client_
                      ->PublishIndex(PublishIndexRequest()
                                         .WithIndex(GetTestIndex())
                                         .WithData(data_)
                                         .WithLayerId(GetTestLayer()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_P(IndexLayerClientOnlineTest, DeleteData) {
  auto response = client_
                      ->PublishIndex(PublishIndexRequest()
                                         .WithIndex(GetTestIndex())
                                         .WithData(data_)
                                         .WithLayerId(GetTestLayer()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
  auto index_id = response.GetResult().GetTraceID();

  auto deleteIndexRes =
      client_
          ->DeleteIndexData(
              DeleteIndexDataRequest().WithIndexId(index_id).WithLayerId(
                  GetTestLayer()))
          .GetFuture()
          .get();

  ASSERT_TRUE(deleteIndexRes.IsSuccessful());
}

TEST_P(IndexLayerClientOnlineTest, PublishDataAsync) {
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

TEST_P(IndexLayerClientOnlineTest, UpdateIndex) {
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

TEST_P(IndexLayerClientOnlineTest, PublishNoData) {
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

TEST_P(IndexLayerClientOnlineTest, PublishNoLayer) {
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

class MockHandler {
 public:
  MOCK_METHOD3(CallOperator,
               olp::client::CancellationToken(
                   const olp::network::NetworkRequest& request,
                   const olp::network::NetworkConfig& config,
                   const olp::client::NetworkAsyncCallback& callback));

  olp::client::CancellationToken operator()(
      const olp::network::NetworkRequest& request,
      const olp::network::NetworkConfig& config,
      const olp::client::NetworkAsyncCallback& callback) {
    return CallOperator(request, config, callback);
  }
};

MATCHER_P(IsGetRequest, url, "") {
  return olp::network::NetworkRequest::HttpVerb::GET == arg.Verb() &&
         url == arg.Url() && (!arg.Content() || arg.Content()->empty());
}

MATCHER_P(IsPostRequest, url, "") {
  return olp::network::NetworkRequest::HttpVerb::POST == arg.Verb() &&
         url == arg.Url();
}

MATCHER_P(IsPutRequest, url, "") {
  return olp::network::NetworkRequest::HttpVerb::PUT == arg.Verb() &&
         url == arg.Url();
}

MATCHER_P(IsPutRequestPrefix, url, "") {
  if (olp::network::NetworkRequest::HttpVerb::PUT != arg.Verb()) {
    return false;
  }

  std::string url_string(url);
  auto res =
      std::mismatch(url_string.begin(), url_string.end(), arg.Url().begin());

  return (res.first == url_string.end());
}

MATCHER_P(IsDeleteRequestPrefix, url, "") {
  if (olp::network::NetworkRequest::HttpVerb::DEL != arg.Verb()) {
    return false;
  }

  std::string url_string(url);
  auto res =
      std::mismatch(url_string.begin(), url_string.end(), arg.Url().begin());

  return (res.first == url_string.end());
}

olp::client::NetworkAsyncHandler indexReturnsResponse(
    olp::network::HttpResponse response) {
  return [=](const olp::network::NetworkRequest& request,
             const olp::network::NetworkConfig& config,
             const olp::client::NetworkAsyncCallback& callback)
             -> olp::client::CancellationToken {
    std::thread([=]() { callback(response); }).detach();
    return olp::client::CancellationToken();
  };
}

olp::client::NetworkAsyncHandler indexSetsPromiseWaitsAndReturns(
    std::shared_ptr<std::promise<void>> preSignal,
    std::shared_ptr<std::promise<void>> waitForSignal,
    olp::network::HttpResponse response) {
  return [preSignal, waitForSignal, response](
             const olp::network::NetworkRequest& request,
             const olp::network::NetworkConfig& /*config*/,
             const olp::client::NetworkAsyncCallback& callback)
             -> olp::client::CancellationToken {
    auto completed = std::make_shared<std::atomic_bool>(false);

    std::thread(
        [request, preSignal, waitForSignal, completed, callback, response]() {
          preSignal->set_value();
          waitForSignal->get_future().get();

          if (!completed->exchange(true)) {
            callback(response);
          }
        })
        .detach();

    return olp::client::CancellationToken([request, completed, callback]() {
      if (!completed->exchange(true)) {
        callback({olp::network::Network::ErrorCode::Cancelled, "Cancelled"});
      }
    });
  };
}

class IndexLayerClientMockTest : public IndexLayerClientTestBase {
 protected:
  MockHandler handler_;

  virtual std::shared_ptr<IndexLayerClient> CreateIndexLayerClient() override {
    olp::client::OlpClientSettings client_settings;
    auto handle = [&](const olp::network::NetworkRequest& request,
                      const olp::network::NetworkConfig& config,
                      const olp::client::NetworkAsyncCallback& callback)
        -> olp::client::CancellationToken {
      return handler_(request, config, callback);
    };
    client_settings.network_async_handler = handle;
    SetUpCommonNetworkMockCalls();

    return std::make_shared<IndexLayerClient>(
        olp::client::HRN{GetTestCatalog()}, client_settings);
  }

  void SetUpCommonNetworkMockCalls() {
    // Catch unexpected calls and fail immediatley
    ON_CALL(handler_, CallOperator(testing::_, testing::_, testing::_))
        .WillByDefault(
            testing::DoAll(testing::InvokeWithoutArgs([]() { FAIL(); }),
                           testing::Invoke(indexReturnsResponse({-1, ""}))));

    ON_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG), testing::_,
                                   testing::_))
        .WillByDefault(testing::Invoke(
            indexReturnsResponse({200, HTTP_RESPONSE_LOOKUP_CONFIG})));

    ON_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INDEX), testing::_,
                                   testing::_))
        .WillByDefault(testing::Invoke(
            indexReturnsResponse({200, HTTP_RESPONSE_LOOKUP_INDEX})));
    ON_CALL(handler_,
            CallOperator(IsGetRequest(URL_LOOKUP_BLOB), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            indexReturnsResponse({200, HTTP_RESPONSE_LOOKUP_BLOB})));

    ON_CALL(handler_,
            CallOperator(IsGetRequest(URL_GET_CATALOG), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            indexReturnsResponse({200, HTTP_RESPONSE_GET_CATALOG})));

    ON_CALL(handler_,
            CallOperator(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX),
                         testing::_, testing::_))
        .WillByDefault(testing::Invoke(indexReturnsResponse({200, ""})));

    ON_CALL(handler_, CallOperator(IsPostRequest(URL_INSERT_INDEX), testing::_,
                                   testing::_))
        .WillByDefault(testing::Invoke(indexReturnsResponse({201, ""})));

    ON_CALL(handler_,
            CallOperator(IsDeleteRequestPrefix(URL_DELETE_BLOB_INDEX_PREFIX),
                         testing::_, testing::_))
        .WillByDefault(testing::Invoke(indexReturnsResponse({200, ""})));
    ON_CALL(handler_, CallOperator(IsPutRequest(URL_INSERT_INDEX), testing::_,
                                   testing::_))
        .WillByDefault(testing::Invoke(indexReturnsResponse({200, ""})));
  }
};

INSTANTIATE_TEST_SUITE_P(TestMock, IndexLayerClientMockTest,
                         ::testing::Values(false));

TEST_P(IndexLayerClientMockTest, PublishData) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_BLOB),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INDEX),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);

    EXPECT_CALL(handler_,
                CallOperator(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX),
                             testing::_, testing::_))
        .Times(1);

    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INSERT_INDEX),
                                       testing::_, testing::_))
        .Times(1);
  }

  auto response = client_
                      ->PublishIndex(PublishIndexRequest()
                                         .WithIndex(GetTestIndex())
                                         .WithData(data_)
                                         .WithLayerId(GetTestLayer()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_P(IndexLayerClientMockTest, DeleteData) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_BLOB),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INDEX),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);

    EXPECT_CALL(handler_,
                CallOperator(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX),
                             testing::_, testing::_))
        .Times(1);

    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INSERT_INDEX),
                                       testing::_, testing::_))
        .Times(1);

    EXPECT_CALL(
        handler_,
        CallOperator(IsDeleteRequestPrefix(URL_DELETE_BLOB_INDEX_PREFIX),
                     testing::_, testing::_))
        .Times(1);
  }

  auto response = client_
                      ->PublishIndex(PublishIndexRequest()
                                         .WithIndex(GetTestIndex())
                                         .WithData(data_)
                                         .WithLayerId(GetTestLayer()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));

  auto index_id = response.GetResult().GetTraceID();

  auto deleteIndexRes =
      client_
          ->DeleteIndexData(
              DeleteIndexDataRequest().WithIndexId(index_id).WithLayerId(
                  GetTestLayer()))
          .GetFuture()
          .get();

  ASSERT_TRUE(deleteIndexRes.IsSuccessful());
}

TEST_P(IndexLayerClientMockTest, UpdateIndex) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_BLOB),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INDEX),
                                       testing::_, testing::_))
        .Times(1);

    EXPECT_CALL(handler_, CallOperator(IsPutRequest(URL_INSERT_INDEX),
                                       testing::_, testing::_))
        .Times(1);
  }

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

  ASSERT_TRUE(response.IsSuccessful());
}

TEST_P(IndexLayerClientMockTest, PublishDataCancelConfig) {
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                     testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(indexSetsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_LOOKUP_CONFIG})));
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_BLOB), testing::_,
                                     testing::_))
      .Times(0);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INDEX), testing::_,
                                     testing::_))
      .Times(0);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG), testing::_,
                                     testing::_))
      .Times(0);

  EXPECT_CALL(handler_,
              CallOperator(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX),
                           testing::_, testing::_))
      .Times(0);

  EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INSERT_INDEX),
                                     testing::_, testing::_))
      .Times(0);

  auto promise = client_->PublishIndex(PublishIndexRequest()
                                           .WithIndex(GetTestIndex())
                                           .WithData(data_)
                                           .WithLayerId(GetTestLayer()));

  waitForCancel->get_future().get();
  promise.GetCancellationToken().cancel();
  pauseForCancel->set_value();

  auto response = promise.GetFuture().get();

  ASSERT_NO_FATAL_FAILURE(PublishCancelledAssertions(response));
}

TEST_P(IndexLayerClientMockTest, PublishDataCancelBlob) {
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                     testing::_, testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_BLOB), testing::_,
                                     testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(indexSetsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_LOOKUP_BLOB})));
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INDEX), testing::_,
                                     testing::_))
      .Times(0);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG), testing::_,
                                     testing::_))
      .Times(0);

  EXPECT_CALL(handler_,
              CallOperator(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX),
                           testing::_, testing::_))
      .Times(0);

  EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INSERT_INDEX),
                                     testing::_, testing::_))
      .Times(0);

  auto promise = client_->PublishIndex(PublishIndexRequest()
                                           .WithIndex(GetTestIndex())
                                           .WithData(data_)
                                           .WithLayerId(GetTestLayer()));

  waitForCancel->get_future().get();
  promise.GetCancellationToken().cancel();
  pauseForCancel->set_value();

  auto response = promise.GetFuture().get();

  ASSERT_NO_FATAL_FAILURE(PublishCancelledAssertions(response));
}

TEST_P(IndexLayerClientMockTest, PublishDataCancelIndex) {
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                     testing::_, testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_BLOB), testing::_,
                                     testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INDEX), testing::_,
                                     testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(indexSetsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_LOOKUP_INDEX})));
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG), testing::_,
                                     testing::_))
      .Times(0);

  EXPECT_CALL(handler_,
              CallOperator(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX),
                           testing::_, testing::_))
      .Times(0);

  EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INSERT_INDEX),
                                     testing::_, testing::_))
      .Times(0);

  auto promise = client_->PublishIndex(PublishIndexRequest()
                                           .WithIndex(GetTestIndex())
                                           .WithData(data_)
                                           .WithLayerId(GetTestLayer()));

  waitForCancel->get_future().get();
  promise.GetCancellationToken().cancel();
  pauseForCancel->set_value();

  auto response = promise.GetFuture().get();

  ASSERT_NO_FATAL_FAILURE(PublishCancelledAssertions(response));
}

TEST_P(IndexLayerClientMockTest, PublishDataCancelGetCatalog) {
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                     testing::_, testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_BLOB), testing::_,
                                     testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INDEX), testing::_,
                                     testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG), testing::_,
                                     testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(indexSetsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_GET_CATALOG})));

  EXPECT_CALL(handler_,
              CallOperator(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX),
                           testing::_, testing::_))
      .Times(0);

  EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INSERT_INDEX),
                                     testing::_, testing::_))
      .Times(0);

  auto promise = client_->PublishIndex(PublishIndexRequest()
                                           .WithIndex(GetTestIndex())
                                           .WithData(data_)
                                           .WithLayerId(GetTestLayer()));

  waitForCancel->get_future().get();
  promise.GetCancellationToken().cancel();
  pauseForCancel->set_value();

  auto response = promise.GetFuture().get();

  ASSERT_NO_FATAL_FAILURE(PublishCancelledAssertions(response));
}

TEST_P(IndexLayerClientMockTest, PublishDataCancelPutBlob) {
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                     testing::_, testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_BLOB), testing::_,
                                     testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INDEX), testing::_,
                                     testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG), testing::_,
                                     testing::_))
      .Times(1);

  EXPECT_CALL(handler_,
              CallOperator(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX),
                           testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(indexSetsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, "OK"})));

  EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INSERT_INDEX),
                                     testing::_, testing::_))
      .Times(0);

  auto promise = client_->PublishIndex(PublishIndexRequest()
                                           .WithIndex(GetTestIndex())
                                           .WithData(data_)
                                           .WithLayerId(GetTestLayer()));

  waitForCancel->get_future().get();
  promise.GetCancellationToken().cancel();
  pauseForCancel->set_value();

  auto response = promise.GetFuture().get();

  ASSERT_NO_FATAL_FAILURE(PublishCancelledAssertions(response));
}
