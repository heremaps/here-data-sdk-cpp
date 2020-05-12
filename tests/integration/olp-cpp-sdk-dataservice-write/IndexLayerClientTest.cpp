/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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
#include <olp/core/client/ApiError.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/write/IndexLayerClient.h>
#include <olp/dataservice/write/model/PublishIndexRequest.h>
#include "HttpResponses.h"

namespace {

using ::testing::_;
using namespace olp::dataservice::write;
using namespace olp::dataservice::write::model;
using namespace olp::tests::common;

void PublishDataSuccessAssertions(
    const olp::client::ApiResponse<
        olp::dataservice::write::model::ResponseOkSingle,
        olp::client::ApiError>& result) {
  EXPECT_TRUE(result.IsSuccessful());
  EXPECT_FALSE(result.GetResult().GetTraceID().empty());
  EXPECT_EQ("", result.GetError().GetMessage());
}

template <typename T>
void PublishCancelledAssertions(
    const olp::client::ApiResponse<T, olp::client::ApiError>& result) {
  EXPECT_FALSE(result.IsSuccessful());
  EXPECT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            result.GetError().GetHttpStatusCode());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            result.GetError().GetErrorCode());
  EXPECT_EQ("Cancelled", result.GetError().GetMessage());
}

class IndexLayerClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    client_ = CreateIndexLayerClient();
    data_ = GenerateData();
  }

  void TearDown() override {
    data_.reset();
    client_.reset();
  }

  std::string GetTestCatalog() {
    return "hrn:here:data::olp-here-test:olp-cpp-sdk-ingestion-test-catalog";
  }

  std::string GetTestLayer() {
    return "olp-cpp-sdk-ingestion-test-index-layer";
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

  virtual std::shared_ptr<IndexLayerClient> CreateIndexLayerClient() {
    olp::client::OlpClientSettings client_settings;
    network_ = std::make_shared<NetworkMock>();
    client_settings.network_request_handler = network_;
    SetUpCommonNetworkMockCalls(*network_);

    return std::make_shared<IndexLayerClient>(
        olp::client::HRN{GetTestCatalog()}, client_settings);
  }

  void SetUpCommonNetworkMockCalls(NetworkMock& network) {
    // Catch unexpected calls and fail immediatley
    ON_CALL(network, Send(_, _, _, _, _))
        .WillByDefault(testing::DoAll(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(-1), ""),
            [](olp::http::NetworkRequest /*request*/,
               olp::http::Network::Payload /*payload*/,
               olp::http::Network::Callback /*callback*/,
               olp::http::Network::HeaderCallback /*header_callback*/,
               olp::http::Network::DataCallback /*data_callback*/) {
              auto fail_helper = []() { FAIL(); };
              fail_helper();
              return olp::http::SendOutcome(5);
            }));
    ON_CALL(network, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_LOOKUP_CONFIG));

    ON_CALL(network, Send(IsGetRequest(URL_LOOKUP_INDEX), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_LOOKUP_INDEX));

    ON_CALL(network, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_LOOKUP_BLOB));

    ON_CALL(network, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_GET_CATALOG));

    ON_CALL(network,
            Send(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200), ""));

    ON_CALL(network, Send(IsPostRequest(URL_INSERT_INDEX), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(201), ""));

    ON_CALL(network, Send(IsDeleteRequestPrefix(URL_DELETE_BLOB_INDEX_PREFIX),
                          _, _, _, _))
        .WillByDefault(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200), ""));

    ON_CALL(network, Send(IsPutRequest(URL_INSERT_INDEX), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200), ""));
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
  std::shared_ptr<NetworkMock> network_;
  std::shared_ptr<IndexLayerClient> client_;
  std::shared_ptr<std::vector<unsigned char>> data_;
};

TEST_F(IndexLayerClientTest, PublishData) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INDEX), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_,
                Send(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsPostRequest(URL_INSERT_INDEX), _, _, _, _))
        .Times(1);
  }

  auto response = client_
                      ->PublishIndex(PublishIndexRequest()
                                         .WithIndex(GetTestIndex())
                                         .WithData(data_)
                                         .WithLayerId(GetTestLayer()))
                      .GetFuture()
                      .get();

  testing::Mock::VerifyAndClearExpectations(network_.get());
  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_F(IndexLayerClientTest, DeleteData) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INDEX), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_,
                Send(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsPostRequest(URL_INSERT_INDEX), _, _, _, _))
        .Times(1);
    EXPECT_CALL(
        *network_,
        Send(IsDeleteRequestPrefix(URL_DELETE_BLOB_INDEX_PREFIX), _, _, _, _))
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

  auto delete_index_response =
      client_
          ->DeleteIndexData(
              DeleteIndexDataRequest().WithIndexId(index_id).WithLayerId(
                  GetTestLayer()))
          .GetFuture()
          .get();

  testing::Mock::VerifyAndClearExpectations(network_.get());
  ASSERT_TRUE(delete_index_response.IsSuccessful());
}

TEST_F(IndexLayerClientTest, UpdateIndex) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INDEX), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsPutRequest(URL_INSERT_INDEX), _, _, _, _))
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

  testing::Mock::VerifyAndClearExpectations(network_.get());
  ASSERT_TRUE(response.IsSuccessful());
}

TEST_F(IndexLayerClientTest, PublishDataCancelConfig) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_CONFIG});
  {
    testing::InSequence dummy;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INDEX), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_,
                Send(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_, Send(IsPostRequest(URL_INSERT_INDEX), _, _, _, _))
        .Times(0);
  }
  auto promise = client_->PublishIndex(PublishIndexRequest()
                                           .WithIndex(GetTestIndex())
                                           .WithData(data_)
                                           .WithLayerId(GetTestLayer()));
  wait_for_cancel->get_future().get();
  promise.GetCancellationToken().Cancel();
  pause_for_cancel->set_value();

  auto response = promise.GetFuture().get();

  testing::Mock::VerifyAndClearExpectations(network_.get());
  ASSERT_NO_FATAL_FAILURE(PublishCancelledAssertions(response));
}

TEST_F(IndexLayerClientTest, PublishDataCancelBlob) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_BLOB});
  {
    testing::InSequence dummy;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INDEX), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_,
                Send(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_, Send(IsPostRequest(URL_INSERT_INDEX), _, _, _, _))
        .Times(0);
  }
  auto promise = client_->PublishIndex(PublishIndexRequest()
                                           .WithIndex(GetTestIndex())
                                           .WithData(data_)
                                           .WithLayerId(GetTestLayer()));

  wait_for_cancel->get_future().get();
  promise.GetCancellationToken().Cancel();
  pause_for_cancel->set_value();

  auto response = promise.GetFuture().get();

  testing::Mock::VerifyAndClearExpectations(network_.get());
  ASSERT_NO_FATAL_FAILURE(PublishCancelledAssertions(response));
}

TEST_F(IndexLayerClientTest, PublishDataCancelIndex) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_INDEX});
  {
    testing::InSequence dummy;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INDEX), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_,
                Send(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_, Send(IsPostRequest(URL_INSERT_INDEX), _, _, _, _))
        .Times(0);
  }
  auto promise = client_->PublishIndex(PublishIndexRequest()
                                           .WithIndex(GetTestIndex())
                                           .WithData(data_)
                                           .WithLayerId(GetTestLayer()));

  wait_for_cancel->get_future().get();
  promise.GetCancellationToken().Cancel();
  pause_for_cancel->set_value();

  auto response = promise.GetFuture().get();

  testing::Mock::VerifyAndClearExpectations(network_.get());
  ASSERT_NO_FATAL_FAILURE(PublishCancelledAssertions(response));
}

TEST_F(IndexLayerClientTest, PublishDataCancelGetCatalog) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_GET_CATALOG});
  {
    testing::InSequence dummy;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INDEX), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
    EXPECT_CALL(*network_,
                Send(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_, Send(IsPostRequest(URL_INSERT_INDEX), _, _, _, _))
        .Times(0);
  }
  auto promise = client_->PublishIndex(PublishIndexRequest()
                                           .WithIndex(GetTestIndex())
                                           .WithData(data_)
                                           .WithLayerId(GetTestLayer()));

  wait_for_cancel->get_future().get();
  promise.GetCancellationToken().Cancel();
  pause_for_cancel->set_value();

  auto response = promise.GetFuture().get();

  testing::Mock::VerifyAndClearExpectations(network_.get());
  ASSERT_NO_FATAL_FAILURE(PublishCancelledAssertions(response));
}

TEST_F(IndexLayerClientTest, PublishDataCancelPutBlob) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, "OK"});
  {
    testing::InSequence dummy;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INDEX), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_,
                Send(IsPutRequestPrefix(URL_PUT_BLOB_INDEX_PREFIX), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
    EXPECT_CALL(*network_, Send(IsPostRequest(URL_INSERT_INDEX), _, _, _, _))
        .Times(0);
  }
  auto promise = client_->PublishIndex(PublishIndexRequest()
                                           .WithIndex(GetTestIndex())
                                           .WithData(data_)
                                           .WithLayerId(GetTestLayer()));

  wait_for_cancel->get_future().get();
  promise.GetCancellationToken().Cancel();
  pause_for_cancel->set_value();

  auto response = promise.GetFuture().get();

  testing::Mock::VerifyAndClearExpectations(network_.get());
  ASSERT_NO_FATAL_FAILURE(PublishCancelledAssertions(response));
}

}  // namespace
