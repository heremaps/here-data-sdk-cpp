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
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/write/StreamLayerClient.h>
#include <olp/dataservice/write/model/PublishDataRequest.h>
#include "HttpResponses.h"

namespace {

using namespace olp::dataservice::write;
using namespace olp::dataservice::write::model;
using namespace testing;
using namespace olp::tests::common;

// Binary SDII Message List protobuf data. See the OLP SDII data specification
// and schema documents to learn about the format. This char array was created
// using the `xxd -i` unix command on the encoded data file. The data was
// encoded using the `protoc` command line tool which is part of a standard
// protobuf system installation.
constexpr unsigned char kSDIITestData[] = {
    0x0a, 0x67, 0x0a, 0x34, 0x0a, 0x05, 0x33, 0x2e, 0x33, 0x2e, 0x32, 0x12,
    0x05, 0x53, 0x49, 0x4d, 0x50, 0x4c, 0x4a, 0x24, 0x31, 0x36, 0x38, 0x64,
    0x38, 0x33, 0x61, 0x65, 0x2d, 0x31, 0x39, 0x63, 0x66, 0x2d, 0x34, 0x62,
    0x38, 0x61, 0x2d, 0x39, 0x30, 0x37, 0x36, 0x2d, 0x66, 0x30, 0x37, 0x38,
    0x35, 0x31, 0x61, 0x35, 0x61, 0x35, 0x31, 0x30, 0x12, 0x2f, 0x0a, 0x2d,
    0x08, 0xb4, 0xda, 0xbd, 0x92, 0xd0, 0x2c, 0x10, 0x01, 0x21, 0xa6, 0x7b,
    0x42, 0x1b, 0x25, 0xec, 0x27, 0x40, 0x29, 0x68, 0xf2, 0x83, 0xa9, 0x1c,
    0x14, 0x48, 0x40, 0x31, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x69, 0xf8, 0xc0,
    0x49, 0xe5, 0x35, 0x94, 0xd7, 0x50, 0x5e, 0x32, 0x40};

constexpr unsigned int kSDIITestDataLength = 105;

void PublishDataSuccessAssertions(
    const olp::client::ApiResponse<ResponseOkSingle, olp::client::ApiError>&
        result) {
  EXPECT_TRUE(result.IsSuccessful());
  EXPECT_FALSE(result.GetResult().GetTraceID().empty());
}

void PublishDataCancelledAssertions(
    const olp::client::ApiResponse<ResponseOkSingle, olp::client::ApiError>&
        result) {
  EXPECT_FALSE(result.IsSuccessful());
  EXPECT_TRUE(result.GetResult().GetTraceID().empty());
  EXPECT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            result.GetError().GetHttpStatusCode());
  EXPECT_EQ(olp::client::ErrorCode::Cancelled,
            result.GetError().GetErrorCode());
}

template <typename T>
void PublishFailureAssertions(
    const olp::client::ApiResponse<T, olp::client::ApiError>& result) {
  EXPECT_FALSE(result.IsSuccessful());
  EXPECT_NE(result.GetError().GetHttpStatusCode(), 200);
  // EXPECT_FALSE(result.GetError().GetMessage().empty());
}

class StreamLayerClientCacheTest : public ::testing::Test {
 protected:
  StreamLayerClientCacheTest() {
    sdii_data_ = std::make_shared<std::vector<unsigned char>>(
        kSDIITestData, kSDIITestData + kSDIITestDataLength);
  }

  ~StreamLayerClientCacheTest() = default;

  void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(client_ = CreateStreamLayerClient());
    data_ = GenerateData();
  }

  void TearDown() override {
    if (disk_cache_) {
      disk_cache_->Close();
    }
    testing::Mock::VerifyAndClearExpectations(network_.get());
    data_.reset();
    client_.reset();
  }

  std::string GetTestCatalog() {
    return "hrn:here:data::olp-here-test:olp-cpp-sdk-ingestion-test-catalog";
  }

  std::string GetTestLayer() {
    return "olp-cpp-sdk-ingestion-test-stream-layer";
  }

  std::string GetTestLayer2() {
    return "olp-cpp-sdk-ingestion-test-stream-layer-2";
  }

  std::string GetTestLayerSdii() {
    return "olp-cpp-sdk-ingestion-test-stream-layer-sdii";
  }

  void QueueMultipleEvents(int num_events) {
    for (int i = 0; i < num_events; i++) {
      data_->push_back(' ');
      data_->push_back(i);
      auto error = client_->Queue(
          PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));
      ASSERT_FALSE(error) << error.get();
    }
  }

  virtual std::shared_ptr<StreamLayerClient> CreateStreamLayerClient() {
    olp::client::OlpClientSettings client_settings;

    disk_cache_ = std::make_shared<olp::cache::DefaultCache>();
    EXPECT_EQ(disk_cache_->Open(),
              olp::cache::DefaultCache::StorageOpenResult::Success);

    network_ = std::make_shared<NetworkMock>();
    client_settings.network_request_handler = network_;
    client_settings.cache = disk_cache_;
    SetUpCommonNetworkMockCalls(*network_);

    return std::make_shared<StreamLayerClient>(
        olp::client::HRN{GetTestCatalog()}, stream_client_settings_,
        client_settings);
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

    ON_CALL(network, Send(IsGetRequest(URL_LOOKUP_INGEST), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_LOOKUP_INGEST));

    ON_CALL(network, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_LOOKUP_CONFIG));

    ON_CALL(network, Send(IsGetRequest(URL_LOOKUP_PUBLISH_V2), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_LOOKUP_PUBLISH_V2));

    ON_CALL(network, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_LOOKUP_BLOB));

    ON_CALL(network,
            Send(testing::AnyOf(IsGetRequest(URL_GET_CATALOG),
                                IsGetRequest(URL_GET_CATALOG_BILLING_TAG)),
                 _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_GET_CATALOG));

    ON_CALL(network,
            Send(testing::AnyOf(IsPostRequest(URL_INGEST_DATA),
                                IsPostRequest(URL_INGEST_DATA_BILLING_TAG)),
                 _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_INGEST_DATA));

    ON_CALL(network, Send(IsPostRequest(URL_INGEST_DATA_LAYER_2), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_INGEST_DATA_LAYER_2));

    ON_CALL(network, Send(IsPostRequest(URL_INIT_PUBLICATION), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_INIT_PUBLICATION));

    ON_CALL(network, Send(IsPutRequestPrefix(URL_PUT_BLOB_PREFIX), _, _, _, _))
        .WillByDefault(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200), ""));

    ON_CALL(network, Send(testing::AnyOf(IsPostRequest(URL_UPLOAD_PARTITIONS),
                                         IsPutRequest(URL_SUBMIT_PUBLICATION)),
                          _, _, _, _))
        .WillByDefault(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(204), ""));

    ON_CALL(network,
            Send(testing::AnyOf(IsPostRequest(URL_INGEST_SDII),
                                IsPostRequest(URL_INGEST_SDII_BILLING_TAG)),
                 _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_INGEST_SDII));
  }

  void FlushDataOnSettingSuccessAssertions(
      const boost::optional<int>& max_events_per_flush = boost::none) {
    for (int i = 0; i < 5; i++) {
      data_->push_back(' ');
      data_->push_back(i);
      auto error = client_->Queue(
          PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));
      EXPECT_FALSE(error) << error.get();
    }

    const auto num_requests_to_flush =
        max_events_per_flush ? *max_events_per_flush : 0;
    auto flush_request = model::FlushRequest().WithNumberOfRequestsToFlush(
        num_requests_to_flush);

    auto response = client_->Flush(std::move(flush_request)).GetFuture().get();
    if (!max_events_per_flush || *max_events_per_flush > 5) {
      EXPECT_EQ(5, response.size());
    } else if (*max_events_per_flush <= 0) {
      EXPECT_TRUE(response.empty());
    } else {
      EXPECT_EQ(*max_events_per_flush, response.size());
    }

    for (auto& single_response : response) {
      EXPECT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(single_response));
    }
  }
  void MaximumRequestsSuccessAssertions(const int maximum_requests,
                                        int num_requests = 0) {
    std::string expected_error = "Maximum number of requests has reached";
    if (num_requests) {
      if (num_requests > maximum_requests) {
        EXPECT_NO_FATAL_FAILURE(QueueMultipleEvents(maximum_requests));
        while (num_requests > maximum_requests) {
          auto error = client_->Queue(
              PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));
          EXPECT_TRUE(error);
          ASSERT_EQ(expected_error.compare(*error), 0);
          num_requests--;
        }
      }
    } else if (maximum_requests) {
      EXPECT_NO_FATAL_FAILURE(QueueMultipleEvents(maximum_requests));
      auto error = client_->Queue(
          PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));
      EXPECT_TRUE(error);
      ASSERT_EQ(expected_error.compare(*error), 0);
    }
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
  std::shared_ptr<olp::cache::DefaultCache> disk_cache_;
  StreamLayerClientSettings stream_client_settings_;
  std::shared_ptr<NetworkMock> network_;
  std::shared_ptr<StreamLayerClient> client_;
  std::shared_ptr<std::vector<unsigned char>> data_;
  std::shared_ptr<std::vector<unsigned char>> sdii_data_;
};

TEST_F(StreamLayerClientCacheTest, FlushDataSingle) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INGEST), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsPostRequest(URL_INGEST_DATA), _, _, _, _))
        .Times(1);
  }

  auto error = client_->Queue(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  ASSERT_FALSE(error) << error.get();

  auto response = client_->Flush(model::FlushRequest()).GetFuture().get();

  ASSERT_FALSE(response.empty());
  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response[0]));
}

TEST_F(StreamLayerClientCacheTest, FlushDataMultiple) {
  EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1);
  EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
      .Times(5);
  EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INGEST), _, _, _, _))
      .Times(1);
  EXPECT_CALL(*network_, Send(IsPostRequest(URL_INGEST_DATA), _, _, _, _))
      .Times(5);

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(5));

  auto response = client_->Flush(model::FlushRequest()).GetFuture().get();

  ASSERT_EQ(5, response.size());
  for (auto& single_response : response) {
    ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(single_response));
  }
}

TEST_F(StreamLayerClientCacheTest, FlushDataCancel) {
  using PromisePtr = std::shared_ptr<std::promise<void>>;
  auto setup_network_expectations_on_cancel =
      [](std::shared_ptr<NetworkMock> network)
      -> std::pair<PromisePtr, PromisePtr> {
    auto wait_for_cancel = std::make_shared<std::promise<void>>();
    auto pause_for_cancel = std::make_shared<std::promise<void>>();

    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        wait_for_cancel, pause_for_cancel,
        {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_INGEST});

    {
      EXPECT_CALL(*network, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
          .Times(1);
      EXPECT_CALL(*network, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
          .Times(1);
      EXPECT_CALL(*network, Send(IsGetRequest(URL_LOOKUP_INGEST), _, _, _, _))
          .Times(1)
          .WillOnce(testing::Invoke(std::move(send_mock)));
      EXPECT_CALL(*network, Cancel(request_id))
          .WillOnce(testing::Invoke(std::move(cancel_mock)));
    }

    return {wait_for_cancel, pause_for_cancel};
  };

  auto network = std::make_shared<NetworkMock>();
  olp::client::OlpClientSettings client_settings;
  client_settings.network_request_handler = network;
  client_settings.task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
  auto kHrn = olp::client::HRN{GetTestCatalog()};

  PublishDataRequest publish_request =
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer());

  {
    SCOPED_TRACE("Cancel Flush via cancellation token");

    SetUpCommonNetworkMockCalls(*network);
    auto promises = setup_network_expectations_on_cancel(network);
    auto wait_for_cancel = promises.first;
    auto pause_for_cancel = promises.second;

    auto client = std::make_shared<StreamLayerClient>(
        kHrn, StreamLayerClientSettings{}, client_settings);
    auto error = client->Queue(publish_request);

    EXPECT_FALSE(error) << error.get();

    auto promise = client->Flush(model::FlushRequest());
    wait_for_cancel->get_future().get();
    promise.GetCancellationToken().Cancel();
    pause_for_cancel->set_value();

    auto response = promise.GetFuture().get();

    EXPECT_EQ(1, response.size());
    EXPECT_NO_FATAL_FAILURE(PublishDataCancelledAssertions(response[0]));

    Mock::VerifyAndClearExpectations(network.get());
  }

  {
    SCOPED_TRACE("Cancel Flush on client destroy");

    SetUpCommonNetworkMockCalls(*network);
    auto promises = setup_network_expectations_on_cancel(network);
    auto wait_for_cancel = promises.first;
    auto pause_for_cancel = promises.second;

    auto client = std::make_shared<StreamLayerClient>(
        kHrn, StreamLayerClientSettings{}, client_settings);
    auto error = client->Queue(publish_request);

    EXPECT_FALSE(error) << error.get();

    auto promise = client->Flush(model::FlushRequest());
    wait_for_cancel->get_future().get();
    client.reset();
    pause_for_cancel->set_value();

    auto response = promise.GetFuture().get();

    EXPECT_EQ(1, response.size());
    EXPECT_NO_FATAL_FAILURE(PublishDataCancelledAssertions(response[0]));

    Mock::VerifyAndClearExpectations(network.get());
  }
}

TEST_F(StreamLayerClientCacheTest, FlushDataMaxEventsDefaultSetting) {
  EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1);
  EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
      .Times(5);
  EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INGEST), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_, Send(IsPostRequest(URL_INGEST_DATA), _, _, _, _))
      .Times(5);

  ASSERT_NO_FATAL_FAILURE(FlushDataOnSettingSuccessAssertions());
}

TEST_F(StreamLayerClientCacheTest, FlushDataMaxEventsValidCustomSetting) {
  const int max_events_per_flush = 3;
  disk_cache_->Close();
  client_ = CreateStreamLayerClient();

  EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1);
  EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
      .Times(3);
  EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INGEST), _, _, _, _))
      .Times(1);
  EXPECT_CALL(*network_, Send(IsPostRequest(URL_INGEST_DATA), _, _, _, _))
      .Times(3);

  ASSERT_NO_FATAL_FAILURE(
      FlushDataOnSettingSuccessAssertions(max_events_per_flush));
}

TEST_F(StreamLayerClientCacheTest, FlushDataMaxEventsInvalidCustomSetting) {
  const int max_events_per_flush = -3;
  disk_cache_->Close();
  client_ = CreateStreamLayerClient();
  {
    testing::InSequence dummy;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INGEST), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_, Send(IsPostRequest(URL_INGEST_DATA), _, _, _, _))
        .Times(0);
  }

  ASSERT_NO_FATAL_FAILURE(
      FlushDataOnSettingSuccessAssertions(max_events_per_flush));
}

TEST_F(StreamLayerClientCacheTest, FlushSettingsMaximumRequests) {
  disk_cache_->Close();
  const auto kMaxRequests = std::numeric_limits<size_t>::max();
  ASSERT_EQ(stream_client_settings_.maximum_requests, kMaxRequests);
  client_ = CreateStreamLayerClient();

  EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1);
  EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
      .Times(15);
  EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_INGEST), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_, Send(IsPostRequest(URL_INGEST_DATA), _, _, _, _))
      .Times(15);

  QueueMultipleEvents(15);
  auto response = client_->Flush(model::FlushRequest()).GetFuture().get();

  ASSERT_EQ(15, response.size());
  for (auto& single_response : response) {
    ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(single_response));
  }
  stream_client_settings_.maximum_requests = 10;
  client_ = CreateStreamLayerClient();
  ASSERT_NO_FATAL_FAILURE(MaximumRequestsSuccessAssertions(10));
  client_ = CreateStreamLayerClient();
  ASSERT_NO_FATAL_FAILURE(MaximumRequestsSuccessAssertions(10, 13));
  client_ = CreateStreamLayerClient();
  ASSERT_NO_FATAL_FAILURE(MaximumRequestsSuccessAssertions(10, 9));
  stream_client_settings_.maximum_requests = 0;
  client_ = CreateStreamLayerClient();
  ASSERT_NO_FATAL_FAILURE(MaximumRequestsSuccessAssertions(0, 10));
}

}  // namespace
