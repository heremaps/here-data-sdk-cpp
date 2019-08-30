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
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef DATASERVICE_WRITE_HAS_OPENSSL
#include <openssl/sha.h>
#endif

#include <olp/authentication/TokenProvider.h>
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/network/HttpResponse.h>

#include <olp/dataservice/write/StreamLayerClient.h>
#include <olp/dataservice/write/model/PublishDataRequest.h>
#include <olp/dataservice/write/model/PublishSdiiRequest.h>

#include "HttpResponses.h"
#include "testutils/CustomParameters.hpp"

using namespace olp::dataservice::write;
using namespace olp::dataservice::write::model;
using namespace testing;

const std::string kEndpoint = "endpoint";
const std::string kAppid = "appid";
const std::string kSecret = "secret";
const std::string kCatalog = "catalog";
const std::string kLayer = "layer";
const std::string kLayer2 = "layer2";
const std::string kLayerSdii = "layer_sdii";

// TODO: Move duplicate test code to common header
namespace {
const std::string kBillingTag = "OlpCppSdkTest";
constexpr int64_t kTwentyMib = 20971520;  // 20 MiB

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

#ifdef DATASERVICE_WRITE_HAS_OPENSSL
// https://stackoverflow.com/a/10632725/275524
std::string sha256(const std::string str) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, str.c_str(), str.size());
  SHA256_Final(hash, &sha256);
  std::stringstream ss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
  }
  return ss.str();
}
#endif

std::string GenerateRandomUUID() {
  static boost::uuids::random_generator gen;
  return boost::uuids::to_string(gen());
}

void PublishDataSuccessAssertions(
    const olp::client::ApiResponse<ResponseOkSingle, olp::client::ApiError>&
        result) {
  EXPECT_TRUE(result.IsSuccessful());
  EXPECT_FALSE(result.GetResult().GetTraceID().empty());
}

void PublishSdiiSuccessAssertions(
    const olp::client::ApiResponse<ResponseOk, olp::client::ApiError>& result) {
  EXPECT_TRUE(result.IsSuccessful());
  EXPECT_FALSE(result.GetResult().GetTraceID().GetParentID().empty());
  ASSERT_FALSE(result.GetResult().GetTraceID().GetGeneratedIDs().empty());
  EXPECT_FALSE(result.GetResult().GetTraceID().GetGeneratedIDs().at(0).empty());
}

template <typename T>
void PublishFailureAssertions(
    const olp::client::ApiResponse<T, olp::client::ApiError>& result) {
  EXPECT_FALSE(result.IsSuccessful());
  EXPECT_NE(result.GetError().GetHttpStatusCode(), 200);
  EXPECT_FALSE(result.GetError().GetMessage().empty());
}
}  // namespace

class StreamLayerClientTestBase : public ::testing::TestWithParam<bool> {
 protected:
  StreamLayerClientTestBase() {
    sdii_data_ = std::make_shared<std::vector<unsigned char>>(
        kSDIITestData, kSDIITestData + kSDIITestDataLength);
  }

  virtual void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(client_ = CreateStreamLayerClient());
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
    return IsOnlineTest() ? CustomParameters::getArgument(kLayer)
                          : "olp-cpp-sdk-ingestion-test-stream-layer";
  }

  std::string GetTestLayer2() {
    return IsOnlineTest() ? CustomParameters::getArgument(kLayer2)
                          : "olp-cpp-sdk-ingestion-test-stream-layer-2";
  }

  std::string GetTestLayerSdii() {
    return IsOnlineTest() ? CustomParameters::getArgument(kLayerSdii)
                          : "olp-cpp-sdk-ingestion-test-stream-layer-sdii";
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

  virtual std::shared_ptr<StreamLayerClient> CreateStreamLayerClient() = 0;

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
  std::shared_ptr<StreamLayerClient> client_;
  std::shared_ptr<std::vector<unsigned char>> data_;
  std::shared_ptr<std::vector<unsigned char>> sdii_data_;
};

class StreamLayerClientOnlineTest : public StreamLayerClientTestBase {
 protected:
  virtual std::shared_ptr<StreamLayerClient> CreateStreamLayerClient()
      override {
    olp::authentication::Settings settings;
    settings.token_endpoint_url = CustomParameters::getArgument(kEndpoint);

    olp::client::OlpClientSettings client_settings;
    client_settings.authentication_settings =
        (olp::client::AuthenticationSettings{
            olp::authentication::TokenProviderDefault{
                CustomParameters::getArgument(kAppid),
                CustomParameters::getArgument(kSecret), settings}});

    return std::make_shared<StreamLayerClient>(
        olp::client::HRN{GetTestCatalog()}, client_settings);
  }
};

INSTANTIATE_TEST_SUITE_P(TestOnline, StreamLayerClientOnlineTest,
                         ::testing::Values(true));

TEST_P(StreamLayerClientOnlineTest, PublishData) {
  auto response =
      client_
          ->PublishData(
              PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()))
          .GetFuture()
          .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_P(StreamLayerClientOnlineTest, PublishDataGreaterThanTwentyMib) {
  auto large_data =
      std::make_shared<std::vector<unsigned char>>(kTwentyMib + 1, 'z');

  auto response = client_
                      ->PublishData(PublishDataRequest()
                                        .WithData(large_data)
                                        .WithLayerId(GetTestLayer()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_P(StreamLayerClientOnlineTest, PublishDataAsync) {
  std::promise<PublishDataResponse> response_promise;
  bool call_is_async = true;

  auto cancel_token = client_->PublishData(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()),
      [&](const PublishDataResponse& response) {
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

TEST_P(StreamLayerClientOnlineTest, PublishDataCancel) {
  auto cancel_future = client_->PublishData(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    cancel_future.GetCancellationToken().cancel();
  })
      .detach();

  auto response = cancel_future.GetFuture().get();

  // If the response is successful, do not fail to avoid flakiness. This could
  // be becauase of a fast network for example.
  if (response.IsSuccessful()) {
    SUCCEED();
    return;
  }

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
  // TODO OlpClient/ API level should be updated to fill in the proper APIError
  // from HttpResponse?
  //  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
  //  response.GetError().GetErrorCode());
}

TEST_P(StreamLayerClientOnlineTest, PublishDataCancelLongDelay) {
  auto cancel_future = client_->PublishData(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    cancel_future.GetCancellationToken().cancel();
  })
      .detach();

  auto response = cancel_future.GetFuture().get();

  // If the response is successful, do not fail to avoid flakiness. This could
  // be becauase of a fast network for example.
  if (response.IsSuccessful()) {
    SUCCEED();
    return;
  }

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
  // TODO OlpClient/ API level should be updated to fill in the proper APIError
  // from HttpResponse?
  //  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
  //  response.GetError().GetErrorCode());
}

TEST_P(StreamLayerClientOnlineTest,
       PublishDataCancelGetFutureAfterRequestCancelled) {
  auto cancel_future = client_->PublishData(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    cancel_future.GetCancellationToken().cancel();
  })
      .detach();

  std::this_thread::sleep_for(std::chrono::milliseconds(400));
  auto response = cancel_future.GetFuture().get();

  // If the response is successful, do not fail to avoid flakiness. This could
  // be becauase of a fast network for example.
  if (response.IsSuccessful()) {
    SUCCEED();
    return;
  }

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
  // TODO OlpClient/ API level should be updated to fill in the proper APIError
  // from HttpResponse?
  //  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
  //  response.GetError().GetErrorCode());
}

TEST_P(StreamLayerClientOnlineTest, PublishDataGreaterThanTwentyMibCancel) {
  auto large_data =
      std::make_shared<std::vector<unsigned char>>(kTwentyMib + 1, 'z');

  auto cancel_future = client_->PublishData(
      PublishDataRequest().WithData(large_data).WithLayerId(GetTestLayer()));

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cancel_future.GetCancellationToken().cancel();
  })
      .detach();

  auto response = cancel_future.GetFuture().get();

  // If the response is successful, do not fail to avoid flakiness. This could
  // be becauase of a fast network for example.
  // TODO(EDE-881) remove the need to do this.
  if (response.IsSuccessful()) {
    SUCCEED();
    return;
  }

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_P(StreamLayerClientOnlineTest, IncorrectLayer) {
  auto response =
      client_
          ->PublishData(
              PublishDataRequest().WithData(data_).WithLayerId("BadLayer"))
          .GetFuture()
          .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_P(StreamLayerClientOnlineTest, NullData) {
  auto response =
      client_
          ->PublishData(PublishDataRequest().WithData(nullptr).WithLayerId(
              GetTestLayer()))
          .GetFuture()
          .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_P(StreamLayerClientOnlineTest, CustomTraceId) {
  const auto uuid = GenerateRandomUUID();

  auto response = client_
                      ->PublishData(PublishDataRequest()
                                        .WithData(data_)
                                        .WithLayerId(GetTestLayer())
                                        .WithTraceId(uuid))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));

  EXPECT_STREQ(response.GetResult().GetTraceID().c_str(), uuid.c_str());
}

TEST_P(StreamLayerClientOnlineTest, BillingTag) {
  auto response = client_
                      ->PublishData(PublishDataRequest()
                                        .WithData(data_)
                                        .WithLayerId(GetTestLayer())
                                        .WithBillingTag(kBillingTag))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

#ifdef DATASERVICE_WRITE_HAS_OPENSSL
TEST_P(StreamLayerClientOnlineTest, ChecksumValid) {
  const std::string data_string(data_->begin(), data_->end());
  auto checksum = sha256(data_string);

  auto response = client_
                      ->PublishData(PublishDataRequest()
                                        .WithData(data_)
                                        .WithLayerId(GetTestLayer())
                                        .WithChecksum(checksum))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}
#endif

TEST_P(StreamLayerClientOnlineTest, ChecksumGarbageString) {
  auto response = client_
                      ->PublishData(PublishDataRequest()
                                        .WithData(data_)
                                        .WithLayerId(GetTestLayer())
                                        .WithChecksum("GarbageChecksum"))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_P(StreamLayerClientOnlineTest, SequentialPublishSameLayer) {
  auto response =
      client_
          ->PublishData(
              PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()))
          .GetFuture()
          .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));

  response = client_
                 ->PublishData(PublishDataRequest().WithData(data_).WithLayerId(
                     GetTestLayer()))
                 .GetFuture()
                 .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_P(StreamLayerClientOnlineTest, SequentialPublishDifferentLayer) {
  auto response =
      client_
          ->PublishData(
              PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()))
          .GetFuture()
          .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));

  response = client_
                 ->PublishData(PublishDataRequest().WithData(data_).WithLayerId(
                     GetTestLayer2()))
                 .GetFuture()
                 .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_P(StreamLayerClientOnlineTest, ConcurrentPublishSameIngestApi) {
  auto publish_data = [&]() {
    auto response =
        client_
            ->PublishData(PublishDataRequest().WithData(data_).WithLayerId(
                GetTestLayer()))
            .GetFuture()
            .get();
    ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
  };

  auto async1 = std::async(std::launch::async, publish_data);
  auto async2 = std::async(std::launch::async, publish_data);
  auto async3 = std::async(std::launch::async, publish_data);
  auto async4 = std::async(std::launch::async, publish_data);
  auto async5 = std::async(std::launch::async, publish_data);

  async1.get();
  async2.get();
  async3.get();
  async4.get();
  async5.get();
}

TEST_P(StreamLayerClientOnlineTest, ConcurrentPublishDifferentIngestApi) {
  auto publishData = [&]() {
    auto client = CreateStreamLayerClient();

    auto response =
        client
            ->PublishData(PublishDataRequest().WithData(data_).WithLayerId(
                GetTestLayer()))
            .GetFuture()
            .get();

    ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
  };

  auto async1 = std::async(std::launch::async, publishData);
  auto async2 = std::async(std::launch::async, publishData);
  auto async3 = std::async(std::launch::async, publishData);
  auto async4 = std::async(std::launch::async, publishData);
  auto async5 = std::async(std::launch::async, publishData);

  async1.get();
  async2.get();
  async3.get();
  async4.get();
  async5.get();
}

TEST_P(StreamLayerClientOnlineTest, PublishSdii) {
  auto response = client_
                      ->PublishSdii(PublishSdiiRequest()
                                        .WithSdiiMessageList(sdii_data_)
                                        .WithLayerId(GetTestLayerSdii()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishSdiiSuccessAssertions(response));
}

TEST_P(StreamLayerClientOnlineTest, PublishSdiiAsync) {
  std::promise<PublishSdiiResponse> response_promise;
  bool call_is_async = true;
  auto cancel_token =
      client_->PublishSdii(PublishSdiiRequest()
                               .WithSdiiMessageList(sdii_data_)
                               .WithLayerId(GetTestLayerSdii()),
                           [&](const PublishSdiiResponse& response) {
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

  ASSERT_NO_FATAL_FAILURE(PublishSdiiSuccessAssertions(response));
}

TEST_P(StreamLayerClientOnlineTest, PublishSdiiCancel) {
  auto cancel_future =
      client_->PublishSdii(PublishSdiiRequest()
                               .WithSdiiMessageList(sdii_data_)
                               .WithLayerId(GetTestLayerSdii()));

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    cancel_future.GetCancellationToken().cancel();
  })
      .detach();

  auto response = cancel_future.GetFuture().get();

  // If the response is successful, do not fail to avoid flakiness. This could
  // be becauase of a fast network for example.
  if (response.IsSuccessful()) {
    SUCCEED();
    return;
  }

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
  // TODO OlpClient/ API level should be updated to fill in the proper APIError
  // from HttpResponse?
  //  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
  //  response.GetError().GetErrorCode());
}

TEST_P(StreamLayerClientOnlineTest, PublishSdiiCancelLongDelay) {
  auto cancel_future =
      client_->PublishSdii(PublishSdiiRequest()
                               .WithSdiiMessageList(sdii_data_)
                               .WithLayerId(GetTestLayerSdii()));

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    cancel_future.GetCancellationToken().cancel();
  })
      .detach();

  auto response = cancel_future.GetFuture().get();

  // If the response is successful, do not fail to avoid flakiness. This could
  // be becauase of a fast network for example.
  if (response.IsSuccessful()) {
    SUCCEED();
    return;
  }

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
  // TODO OlpClient/ API level should be updated to fill in the proper APIError
  // from HttpResponse?
  //  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
  //  response.GetError().GetErrorCode());
}

TEST_P(StreamLayerClientOnlineTest, PublishSDIINonSDIIData) {
  auto response =
      client_
          ->PublishSdii(
              PublishSdiiRequest().WithSdiiMessageList(data_).WithLayerId(
                  GetTestLayerSdii()))
          .GetFuture()
          .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_P(StreamLayerClientOnlineTest, PublishSDIIIncorrectLayer) {
  auto response = client_
                      ->PublishSdii(PublishSdiiRequest()
                                        .WithSdiiMessageList(sdii_data_)
                                        .WithLayerId("BadLayer"))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_P(StreamLayerClientOnlineTest, PublishSDIICustomTraceId) {
  const auto uuid = GenerateRandomUUID();

  auto response = client_
                      ->PublishSdii(PublishSdiiRequest()
                                        .WithSdiiMessageList(sdii_data_)
                                        .WithLayerId(GetTestLayerSdii())
                                        .WithTraceId(uuid))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishSdiiSuccessAssertions(response));

  EXPECT_STREQ(response.GetResult().GetTraceID().GetParentID().c_str(),
               uuid.c_str());
}

TEST_P(StreamLayerClientOnlineTest, PublishSDIIBillingTag) {
  auto response = client_
                      ->PublishSdii(PublishSdiiRequest()
                                        .WithSdiiMessageList(sdii_data_)
                                        .WithLayerId(GetTestLayerSdii())
                                        .WithBillingTag(kBillingTag))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishSdiiSuccessAssertions(response));
}

#ifdef DATASERVICE_WRITE_HAS_OPENSSL
TEST_P(StreamLayerClientOnlineTest, SDIIChecksumValid) {
  const std::string data_string(sdii_data_->begin(), sdii_data_->end());
  auto checksum = sha256(data_string);

  auto response = client_
                      ->PublishSdii(PublishSdiiRequest()
                                        .WithSdiiMessageList(sdii_data_)
                                        .WithLayerId(GetTestLayerSdii())
                                        .WithChecksum(checksum))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishSdiiSuccessAssertions(response));
}
#endif

TEST_P(StreamLayerClientOnlineTest, SDIIChecksumGarbageString) {
  auto response = client_
                      ->PublishSdii(PublishSdiiRequest()
                                        .WithSdiiMessageList(sdii_data_)
                                        .WithLayerId(GetTestLayerSdii())
                                        .WithChecksum("GarbageChecksum"))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_P(StreamLayerClientOnlineTest, SDIIConcurrentPublishSameIngestApi) {
  auto publish_data = [&]() {
    auto response = client_
                        ->PublishSdii(PublishSdiiRequest()
                                          .WithSdiiMessageList(sdii_data_)
                                          .WithLayerId(GetTestLayerSdii()))
                        .GetFuture()
                        .get();
    ASSERT_NO_FATAL_FAILURE(PublishSdiiSuccessAssertions(response));
  };

  auto async1 = std::async(std::launch::async, publish_data);
  auto async2 = std::async(std::launch::async, publish_data);
  auto async3 = std::async(std::launch::async, publish_data);
  auto async4 = std::async(std::launch::async, publish_data);
  auto async5 = std::async(std::launch::async, publish_data);

  async1.get();
  async2.get();
  async3.get();
  async4.get();
  async5.get();
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

olp::client::NetworkAsyncHandler returnsResponse(
    olp::network::HttpResponse response) {
  return [=](const olp::network::NetworkRequest& request,
             const olp::network::NetworkConfig& config,
             const olp::client::NetworkAsyncCallback& callback)
             -> olp::client::CancellationToken {
    std::thread([=]() { callback(response); }).detach();
    return olp::client::CancellationToken();
  };
}

class StreamLayerClientMockTest : public StreamLayerClientTestBase {
 protected:
  MockHandler handler_;

  virtual std::shared_ptr<StreamLayerClient> CreateStreamLayerClient()
      override {
    olp::client::OlpClientSettings client_settings;

    auto handle = [&](const olp::network::NetworkRequest& request,
                      const olp::network::NetworkConfig& config,
                      const olp::client::NetworkAsyncCallback& callback)
        -> olp::client::CancellationToken {
      return handler_(request, config, callback);
    };
    client_settings.network_async_handler = handle;
    SetUpCommonNetworkMockCalls();

    return std::make_shared<StreamLayerClient>(
        olp::client::HRN{GetTestCatalog()}, client_settings);
  }

  void SetUpCommonNetworkMockCalls() {
    // Catch unexpected calls and fail immediately
    ON_CALL(handler_, CallOperator(testing::_, testing::_, testing::_))
        .WillByDefault(
            testing::DoAll(testing::InvokeWithoutArgs([]() { FAIL(); }),
                           testing::Invoke(returnsResponse({-1, ""}))));

    ON_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST), testing::_,
                                   testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_LOOKUP_INGEST})));

    ON_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG), testing::_,
                                   testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_LOOKUP_CONFIG})));

    ON_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_PUBLISH_V2),
                                   testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_LOOKUP_PUBLISH_V2})));

    ON_CALL(handler_,
            CallOperator(IsGetRequest(URL_LOOKUP_BLOB), testing::_, testing::_))
        .WillByDefault(
            testing::Invoke(returnsResponse({200, HTTP_RESPONSE_LOOKUP_BLOB})));

    ON_CALL(
        handler_,
        CallOperator(testing::AnyOf(IsGetRequest(URL_GET_CATALOG),
                                    IsGetRequest(URL_GET_CATALOG_BILLING_TAG)),
                     testing::_, testing::_))
        .WillByDefault(
            testing::Invoke(returnsResponse({200, HTTP_RESPONSE_GET_CATALOG})));

    ON_CALL(
        handler_,
        CallOperator(testing::AnyOf(IsPostRequest(URL_INGEST_DATA),
                                    IsPostRequest(URL_INGEST_DATA_BILLING_TAG)),
                     testing::_, testing::_))
        .WillByDefault(
            testing::Invoke(returnsResponse({200, HTTP_RESPONSE_INGEST_DATA})));

    ON_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA_LAYER_2),
                                   testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_INGEST_DATA_LAYER_2})));

    ON_CALL(handler_, CallOperator(IsPostRequest(URL_INIT_PUBLICATION),
                                   testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_INIT_PUBLICATION})));

    ON_CALL(handler_, CallOperator(IsPutRequestPrefix(URL_PUT_BLOB_PREFIX),
                                   testing::_, testing::_))
        .WillByDefault(testing::Invoke(returnsResponse({200, ""})));

    ON_CALL(handler_,
            CallOperator(testing::AnyOf(IsPostRequest(URL_UPLOAD_PARTITIONS),
                                        IsPutRequest(URL_SUBMIT_PUBLICATION)),
                         testing::_, testing::_))
        .WillByDefault(testing::Invoke(returnsResponse({204, ""})));

    ON_CALL(
        handler_,
        CallOperator(testing::AnyOf(IsPostRequest(URL_INGEST_SDII),
                                    IsPostRequest(URL_INGEST_SDII_BILLING_TAG)),
                     testing::_, testing::_))
        .WillByDefault(
            testing::Invoke(returnsResponse({200, HTTP_RESPONSE_INGEST_SDII})));
  }
};

INSTANTIATE_TEST_SUITE_P(TestMock, StreamLayerClientMockTest,
                         ::testing::Values(false));

TEST_P(StreamLayerClientMockTest, PublishData) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(1);
  }

  auto response =
      client_
          ->PublishData(
              PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()))
          .GetFuture()
          .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_P(StreamLayerClientMockTest, PublishDataGreaterThanTwentyMib) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_PUBLISH_V2),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_BLOB),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INIT_PUBLICATION),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPutRequestPrefix(URL_PUT_BLOB_PREFIX),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_UPLOAD_PARTITIONS),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPutRequest(URL_SUBMIT_PUBLICATION),
                                       testing::_, testing::_))
        .Times(1);
  }

  auto large_data =
      std::make_shared<std::vector<unsigned char>>(kTwentyMib + 1, 'z');

  auto response = client_
                      ->PublishData(PublishDataRequest()
                                        .WithData(large_data)
                                        .WithLayerId(GetTestLayer()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_P(StreamLayerClientMockTest, PublishDataCancel) {
  auto cancel_token = olp::client::CancellationToken();
  ON_CALL(handler_,
          CallOperator(IsGetRequest(URL_LOOKUP_CONFIG), testing::_, testing::_))
      .WillByDefault(testing::DoAll(
          testing::InvokeWithoutArgs(
              [&cancel_token]() { cancel_token.cancel(); }),
          testing::Invoke(
              returnsResponse({200, HTTP_RESPONSE_LOOKUP_CONFIG}))));

  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
  }

  auto cancel_future = client_->PublishData(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));
  cancel_token = cancel_future.GetCancellationToken();
  auto response = cancel_future.GetFuture().get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_P(StreamLayerClientMockTest, PublishDataCancelLongDelay) {
  auto cancel_token = olp::client::CancellationToken();
  ON_CALL(handler_,
          CallOperator(IsGetRequest(URL_GET_CATALOG), testing::_, testing::_))
      .WillByDefault(testing::DoAll(
          testing::InvokeWithoutArgs(
              [&cancel_token]() { cancel_token.cancel(); }),
          testing::Invoke(returnsResponse({200, HTTP_RESPONSE_GET_CATALOG}))));

  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
  }

  auto cancel_future = client_->PublishData(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));
  cancel_token = cancel_future.GetCancellationToken();
  auto response = cancel_future.GetFuture().get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_P(StreamLayerClientMockTest, BillingTag) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_,
                CallOperator(IsGetRequest(URL_GET_CATALOG_BILLING_TAG),
                             testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_,
                CallOperator(IsPostRequest(URL_INGEST_DATA_BILLING_TAG),
                             testing::_, testing::_))
        .Times(1);
  }

  auto response = client_
                      ->PublishData(PublishDataRequest()
                                        .WithData(data_)
                                        .WithLayerId(GetTestLayer())
                                        .WithBillingTag(kBillingTag))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_P(StreamLayerClientMockTest, ConcurrentPublishSameIngestApi) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(5);
  }

  auto publish_data = [&]() {
    auto response =
        client_
            ->PublishData(PublishDataRequest().WithData(data_).WithLayerId(
                GetTestLayer()))
            .GetFuture()
            .get();
    ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
  };

  auto async1 = std::async(std::launch::async, publish_data);
  auto async2 = std::async(std::launch::async, publish_data);
  auto async3 = std::async(std::launch::async, publish_data);
  auto async4 = std::async(std::launch::async, publish_data);
  auto async5 = std::async(std::launch::async, publish_data);

  async1.get();
  async2.get();
  async3.get();
  async4.get();
  async5.get();
}

TEST_P(StreamLayerClientMockTest, SequentialPublishDifferentLayer) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA_LAYER_2),
                                       testing::_, testing::_))
        .Times(1);
  }

  auto response =
      client_
          ->PublishData(
              PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()))
          .GetFuture()
          .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));

  response = client_
                 ->PublishData(PublishDataRequest().WithData(data_).WithLayerId(
                     GetTestLayer2()))
                 .GetFuture()
                 .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_P(StreamLayerClientMockTest, PublishSdii) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_SDII),
                                       testing::_, testing::_))
        .Times(1);
  }

  auto response = client_
                      ->PublishSdii(PublishSdiiRequest()
                                        .WithSdiiMessageList(sdii_data_)
                                        .WithLayerId(GetTestLayerSdii()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishSdiiSuccessAssertions(response));
}

TEST_P(StreamLayerClientMockTest, PublishSDIIBillingTag) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_,
                CallOperator(IsPostRequest(URL_INGEST_SDII_BILLING_TAG),
                             testing::_, testing::_))
        .Times(1);
  }

  auto response = client_
                      ->PublishSdii(PublishSdiiRequest()
                                        .WithSdiiMessageList(sdii_data_)
                                        .WithLayerId(GetTestLayerSdii())
                                        .WithBillingTag(kBillingTag))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishSdiiSuccessAssertions(response));
}

TEST_P(StreamLayerClientMockTest, PublishSdiiCancel) {
  auto cancel_token = olp::client::CancellationToken();

  {
    InSequence s;
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST), _, _))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG), _, _))
        .WillOnce(
            DoAll(InvokeWithoutArgs([&cancel_token] { cancel_token.cancel(); }),
                  Invoke(returnsResponse({200, HTTP_RESPONSE_LOOKUP_CONFIG}))));
  }

  auto cancel_future =
      client_->PublishSdii(PublishSdiiRequest()
                               .WithSdiiMessageList(sdii_data_)
                               .WithLayerId(GetTestLayerSdii()));
  cancel_token = cancel_future.GetCancellationToken();
  auto response = cancel_future.GetFuture().get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_P(StreamLayerClientMockTest, SDIIConcurrentPublishSameIngestApi) {
  {
    testing::InSequence s;
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST), _, _))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG), _, _))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_SDII), _, _))
        .Times(6);
  }

  auto publish_data = [&]() {
    auto response = client_
                        ->PublishSdii(PublishSdiiRequest()
                                          .WithSdiiMessageList(sdii_data_)
                                          .WithLayerId(GetTestLayerSdii()))
                        .GetFuture()
                        .get();
    ASSERT_NO_FATAL_FAILURE(PublishSdiiSuccessAssertions(response));
  };

  // Trigger one call prior to get cache filled else we face
  // flakiness due to missing expects
  publish_data();

  auto async1 = std::async(std::launch::async, publish_data);
  auto async2 = std::async(std::launch::async, publish_data);
  auto async3 = std::async(std::launch::async, publish_data);
  auto async4 = std::async(std::launch::async, publish_data);
  auto async5 = std::async(std::launch::async, publish_data);
  async1.get();
  async2.get();
  async3.get();
  async4.get();
  async5.get();
}

class StreamLayerClientCacheOnlineTest : public StreamLayerClientOnlineTest {
 protected:
  virtual std::shared_ptr<StreamLayerClient> CreateStreamLayerClient()
      override {
    olp::authentication::Settings settings;
    settings.token_endpoint_url = CustomParameters::getArgument(kEndpoint);

    olp::client::OlpClientSettings client_settings;
    client_settings.authentication_settings =
        (olp::client::AuthenticationSettings{
            olp::authentication::TokenProviderDefault{
                CustomParameters::getArgument(kAppid),
                CustomParameters::getArgument(kSecret), settings}});

    disk_cache_ = std::make_shared<olp::cache::DefaultCache>();
    EXPECT_EQ(disk_cache_->Open(),
              olp::cache::DefaultCache::StorageOpenResult::Success);

    return std::make_shared<StreamLayerClient>(
        olp::client::HRN{GetTestCatalog()}, client_settings, disk_cache_,
        flush_settings_);
  }

  virtual void TearDown() override {
    StreamLayerClientTestBase::TearDown();
    if (disk_cache_) {
      disk_cache_->Close();
    }
  }

  std::shared_ptr<olp::cache::DefaultCache> disk_cache_;
  FlushSettings flush_settings_;
};

INSTANTIATE_TEST_SUITE_P(TestCacheOnline, StreamLayerClientCacheOnlineTest,
                         ::testing::Values(true));

TEST_P(StreamLayerClientCacheOnlineTest, Queue) {
  auto error = client_->Queue(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  ASSERT_FALSE(error) << error.get();
}

TEST_P(StreamLayerClientCacheOnlineTest, QueueNullData) {
  auto error = client_->Queue(
      PublishDataRequest().WithData(nullptr).WithLayerId(GetTestLayer()));

  ASSERT_TRUE(error);
}

TEST_P(StreamLayerClientCacheOnlineTest, QueueExtraRequestParams) {
  const auto uuid = GenerateRandomUUID();

  auto error = client_->Queue(PublishDataRequest()
                                  .WithData(data_)
                                  .WithLayerId(GetTestLayer())
                                  .WithTraceId(uuid)
                                  .WithBillingTag(kBillingTag));

  ASSERT_FALSE(error) << error.get();
}

#ifdef DATASERVICE_WRITE_HAS_OPENSSL
TEST_P(StreamLayerClientCacheOnlineTest, QueueWithChecksum) {
  const std::string data_string(data_->begin(), data_->end());
  auto checksum = sha256(data_string);

  auto error = client_->Queue(PublishDataRequest()
                                  .WithData(data_)
                                  .WithLayerId(GetTestLayer())
                                  .WithChecksum(checksum));

  ASSERT_FALSE(error) << error.get();
}
#endif

TEST_P(StreamLayerClientCacheOnlineTest, FlushDataSingle) {
  auto error = client_->Queue(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  ASSERT_FALSE(error) << error.get();

  auto response = client_->Flush().GetFuture().get();

  ASSERT_FALSE(response.empty());
  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response[0]));
}

TEST_P(StreamLayerClientCacheOnlineTest, FlushDataMultiple) {
  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(5));

  auto response = client_->Flush().GetFuture().get();

  ASSERT_EQ(5, response.size());
  for (auto& single_response : response) {
    ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(single_response));
  }
}

TEST_P(StreamLayerClientCacheOnlineTest, FlushDataSingleAsync) {
  auto error = client_->Queue(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  ASSERT_FALSE(error) << error.get();

  std::promise<StreamLayerClient::FlushResponse> response_promise;
  bool call_is_async = true;
  auto cancel_token =
      client_->Flush([&](StreamLayerClient::FlushResponse response) {
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

  ASSERT_FALSE(response.empty());
  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response[0]));
}

TEST_P(StreamLayerClientCacheOnlineTest, FlushDataMultipleAsync) {
  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(5));

  std::promise<StreamLayerClient::FlushResponse> response_promise;
  bool call_is_async = true;
  auto cancel_token =
      client_->Flush([&](StreamLayerClient::FlushResponse response) {
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

  ASSERT_EQ(5, response.size());
  for (auto& single_response : response) {
    ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(single_response));
  }
}

TEST_P(StreamLayerClientCacheOnlineTest, FlushDataCancel) {
  auto error = client_->Queue(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  ASSERT_FALSE(error) << error.get();

  auto cancel_future = client_->Flush();

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    cancel_future.GetCancellationToken().cancel();
  })
      .detach();

  auto response = cancel_future.GetFuture().get();

  ASSERT_EQ(1, response.size());
  if (response[0].IsSuccessful()) {
    SUCCEED();
    return;
  }

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response[0]));
}

TEST_P(StreamLayerClientCacheOnlineTest, FlushListenerMetrics) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 3;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(3));

  auto default_listener = StreamLayerClient::DefaultListener();

  client_->Enable(default_listener);

  for (int i = 0; default_listener->GetNumFlushEvents() < 1; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 200) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(3, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheOnlineTest,
       FlushListenerMetricsSetListenerBeforeQueuing) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 3;
  client_ = CreateStreamLayerClient();

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(3));

  for (int i = 0; default_listener->GetNumFlushEvents() < 1; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 200) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(3, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheOnlineTest, FlushListenerDisable) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 3;
  client_ = CreateStreamLayerClient();

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(3));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  auto disable_future = client_->Disable();
  auto status = disable_future.wait_for(std::chrono::seconds(5));
  if (status != std::future_status::ready) {
    FAIL() << "Timeout waiting for auto flushing to be disabled";
  }
  disable_future.get();

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsFailed());
}

TEST_P(StreamLayerClientCacheOnlineTest,
       FlushListenerMetricsMultipleFlushEventsInSeries) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 2;
  client_ = CreateStreamLayerClient();

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));

  for (int i = 0, j = 1;; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (default_listener->GetNumFlushEvents() == j) {
      if (j == 3) {
        break;
      }
      ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));
      j++;
    }
    if (i > 400) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_EQ(3, default_listener->GetNumFlushEvents());
  EXPECT_EQ(3, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(6, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheOnlineTest,
       FlushListenerMetricsMultipleFlushEventsInParallel) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 2;
  client_ = CreateStreamLayerClient();

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(6));

  for (int i = 0; default_listener->GetNumFlushedRequests() < 6; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 500) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_LE(3, default_listener->GetNumFlushEvents());
  EXPECT_LE(3, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(6, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheOnlineTest,
       FlushListenerMetricsMultipleFlushEventsInParallelStaggeredQueue) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 2;
  client_ = CreateStreamLayerClient();

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(4));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(4));

  for (int i = 0; default_listener->GetNumFlushedRequests() < 10; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 600) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_LE(3, default_listener->GetNumFlushEvents());
  EXPECT_LE(3, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(10, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheOnlineTest, FlushListenerNotifications) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 3;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(3));

  class NotificationListener : public DefaultFlushEventListener<
                                   const StreamLayerClient::FlushResponse&> {
   public:
    void NotifyFlushEventStarted() override { events_started_++; }

    void NotifyFlushEventResults(
        const StreamLayerClient::FlushResponse& results) override {
      std::lock_guard<std::mutex> lock(results_mutex_);
      results_ = std::move(results);
    }

    const StreamLayerClient::FlushResponse& GetResults() {
      std::lock_guard<std::mutex> lock(results_mutex_);
      return results_;
    }

    int events_started_ = 0;

   private:
    StreamLayerClient::FlushResponse results_;
    std::mutex results_mutex_;
  };

  auto notification_listener = std::make_shared<NotificationListener>();
  client_->Enable(notification_listener);

  for (int i = 0; notification_listener->GetResults().size() < 3; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 200) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_EQ(1, notification_listener->events_started_);
  for (auto result : notification_listener->GetResults()) {
    ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(result));
  }
}

TEST_P(StreamLayerClientCacheOnlineTest, FlushSettingsTimeSinceOldRequest) {
  disk_cache_->Close();
  flush_settings_.auto_flush_old_events_force_flush_interval = 10;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  for (int i = 0; default_listener->GetNumFlushEvents() < 1; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 400) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(2, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheOnlineTest,
       FlushSettingsTimeSinceOldRequestQueueAfterEnable) {
  disk_cache_->Close();
  flush_settings_.auto_flush_old_events_force_flush_interval = 10;
  client_ = CreateStreamLayerClient();

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));

  for (int i = 0; default_listener->GetNumFlushEvents() < 1; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 400) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(2, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheOnlineTest,
       FlushSettingsTimeSinceOldRequestDisable) {
  disk_cache_->Close();
  flush_settings_.auto_flush_old_events_force_flush_interval = 2;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  std::this_thread::sleep_for(std::chrono::milliseconds(2100));

  auto disable_future = client_->Disable();
  auto status = disable_future.wait_for(std::chrono::seconds(5));
  if (status != std::future_status::ready) {
    FAIL() << "Timeout waiting for auto flushing to be disabled";
  }
  disable_future.get();

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsFailed());
}

TEST_P(StreamLayerClientCacheOnlineTest, FlushSettingsAutoFlushInterval) {
  disk_cache_->Close();
  flush_settings_.auto_flush_interval = 10;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  for (int i = 0; default_listener->GetNumFlushEvents() < 1; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 400) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(2, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheOnlineTest,
       FlushSettingsAutoFlushIntervalDisable) {
  disk_cache_->Close();
  flush_settings_.auto_flush_interval = 2;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  std::this_thread::sleep_for(std::chrono::milliseconds(2100));

  auto disable_future = client_->Disable();
  auto status = disable_future.wait_for(std::chrono::seconds(5));
  if (status != std::future_status::ready) {
    FAIL() << "Timeout waiting for auto flushing to be disabled";
  }
  disable_future.get();

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsFailed());
}

class StreamLayerClientCacheMockTest : public StreamLayerClientMockTest {
 protected:
  virtual std::shared_ptr<StreamLayerClient> CreateStreamLayerClient()
      override {
    olp::client::OlpClientSettings client_settings;

    disk_cache_ = std::make_shared<olp::cache::DefaultCache>();
    EXPECT_EQ(disk_cache_->Open(),
              olp::cache::DefaultCache::StorageOpenResult::Success);

    auto handle = [&](const olp::network::NetworkRequest& request,
                      const olp::network::NetworkConfig& config,
                      const olp::client::NetworkAsyncCallback& callback)
        -> olp::client::CancellationToken {
      return handler_(request, config, callback);
    };
    client_settings.network_async_handler = handle;
    SetUpCommonNetworkMockCalls();

    return std::make_shared<StreamLayerClient>(
        olp::client::HRN{GetTestCatalog()}, client_settings, disk_cache_,
        flush_settings_);
  }

  virtual void TearDown() override {
    StreamLayerClientTestBase::TearDown();
    if (disk_cache_) {
      disk_cache_->Close();
    }
  }

  void FlushDataOnSettingSuccessAssertions(
      const boost::optional<int>& max_events_per_flush = boost::none) {
    disk_cache_->Close();
    flush_settings_.events_per_single_flush = max_events_per_flush;
    client_ = CreateStreamLayerClient();
    for (int i = 0; i < 5; i++) {
      data_->push_back(' ');
      data_->push_back(i);
      auto error = client_->Queue(
          PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));
      EXPECT_FALSE(error) << error.get();
    }
    auto response = client_->Flush().GetFuture().get();
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
  void maximumRequestsSuccessAssertions(const int maximum_requests,
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

  std::shared_ptr<olp::cache::DefaultCache> disk_cache_;
  FlushSettings flush_settings_;
};

INSTANTIATE_TEST_SUITE_P(TestCacheMock, StreamLayerClientCacheMockTest,
                         ::testing::Values(true));

TEST_P(StreamLayerClientCacheMockTest, FlushDataSingle) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(1);
  }

  auto error = client_->Queue(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  ASSERT_FALSE(error) << error.get();

  auto response = client_->Flush().GetFuture().get();

  ASSERT_FALSE(response.empty());
  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response[0]));
}

TEST_P(StreamLayerClientCacheMockTest, FlushDataMultiple) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(5);
  }

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(5));

  auto response = client_->Flush().GetFuture().get();

  ASSERT_EQ(5, response.size());
  for (auto& single_response : response) {
    ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(single_response));
  }
}

TEST_P(StreamLayerClientCacheMockTest, FlushDataCancel) {
  auto cancel_token = olp::client::CancellationToken();
  ON_CALL(handler_,
          CallOperator(IsGetRequest(URL_LOOKUP_CONFIG), testing::_, testing::_))
      .WillByDefault(testing::DoAll(
          testing::InvokeWithoutArgs(
              [&cancel_token]() { cancel_token.cancel(); }),
          testing::Invoke(
              returnsResponse({200, HTTP_RESPONSE_LOOKUP_CONFIG}))));

  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
  }

  auto error = client_->Queue(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  ASSERT_FALSE(error) << error.get();

  auto cancel_future = client_->Flush();
  cancel_token = cancel_future.GetCancellationToken();
  auto response = cancel_future.GetFuture().get();

  ASSERT_EQ(1, response.size());
  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response[0]));
}

TEST_P(StreamLayerClientCacheMockTest, FlushListenerMetrics) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(3);
  }

  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 3;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(3));

  auto default_listener = StreamLayerClient::DefaultListener();

  client_->Enable(default_listener);

  for (int i = 0; default_listener->GetNumFlushEvents() < 1; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 200) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(3, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheMockTest,
       FlushListenerMetricsSetListenerBeforeQueuing) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(3);
  }

  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 3;
  client_ = CreateStreamLayerClient();

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(3));

  for (int i = 0; default_listener->GetNumFlushEvents() < 1; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 200) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(3, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheMockTest,
       FlushListenerMetricsMultipleFlushEventsInSeries) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(6);
  }

  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 2;
  client_ = CreateStreamLayerClient();

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));

  for (int i = 0, j = 1;; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (default_listener->GetNumFlushEvents() == j) {
      if (j == 3) {
        break;
      }
      ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));
      j++;
    }
    if (i > 400) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_EQ(3, default_listener->GetNumFlushEvents());
  EXPECT_EQ(3, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(6, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheMockTest,
       FlushListenerMetricsMultipleFlushEventsInParallel) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(6);
  }

  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 2;
  client_ = CreateStreamLayerClient();

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(6));

  for (int i = 0; default_listener->GetNumFlushedRequests() < 6; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 200) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_LE(3, default_listener->GetNumFlushEvents());
  EXPECT_LE(3, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(6, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheMockTest, FlushListenerNotifications) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(3);
  }

  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 3;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(3));

  class NotificationListener : public DefaultFlushEventListener<
                                   const StreamLayerClient::FlushResponse&> {
   public:
    void NotifyFlushEventStarted() override { events_started_++; }

    void NotifyFlushEventResults(
        const StreamLayerClient::FlushResponse& results) override {
      std::lock_guard<std::mutex> lock(results_mutex_);
      results_ = std::move(results);
    }

    const StreamLayerClient::FlushResponse& GetResults() {
      std::lock_guard<std::mutex> lock(results_mutex_);
      return results_;
    }

    int events_started_ = 0;

   private:
    StreamLayerClient::FlushResponse results_;
    std::mutex results_mutex_;
  };

  auto notification_listener = std::make_shared<NotificationListener>();
  client_->Enable(notification_listener);

  for (int i = 0; notification_listener->GetResults().size() < 3; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 200) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_EQ(1, notification_listener->events_started_);
  for (auto result : notification_listener->GetResults()) {
    ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(result));
  }
}

TEST_P(StreamLayerClientCacheMockTest, FlushDataMaxEventsDefaultSetting) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(5);
  }
  ASSERT_NO_FATAL_FAILURE(FlushDataOnSettingSuccessAssertions());
}

TEST_P(StreamLayerClientCacheMockTest, FlushDataMaxEventsValidCustomSetting) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(3);
  }

  ASSERT_NO_FATAL_FAILURE(FlushDataOnSettingSuccessAssertions(3));
}

TEST_P(StreamLayerClientCacheMockTest, FlushDataMaxEventsInvalidCustomSetting) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(0);
  }

  ASSERT_NO_FATAL_FAILURE(FlushDataOnSettingSuccessAssertions(-3));
}

TEST_P(StreamLayerClientCacheMockTest, FlushSettingsTimeSinceOldRequest) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(2);
  }

  disk_cache_->Close();
  flush_settings_.auto_flush_old_events_force_flush_interval = 1;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  for (int i = 0; default_listener->GetNumFlushEvents() < 1; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 20) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(2, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheMockTest, FlushSettingsAutoFlushInterval) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(2);
  }

  disk_cache_->Close();
  flush_settings_.auto_flush_interval = 1;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));

  auto default_listener = StreamLayerClient::DefaultListener();
  client_->Enable(default_listener);

  for (int i = 0; default_listener->GetNumFlushEvents() < 1; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 100) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(2, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_P(StreamLayerClientCacheMockTest, FlushSettingsMaximumRequests) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_INGEST),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsPostRequest(URL_INGEST_DATA),
                                       testing::_, testing::_))
        .Times(15);
  }

  disk_cache_->Close();
  ASSERT_EQ(flush_settings_.maximum_requests, boost::none);
  client_ = CreateStreamLayerClient();
  QueueMultipleEvents(15);
  auto response = client_->Flush().GetFuture().get();

  ASSERT_EQ(15, response.size());
  for (auto& single_response : response) {
    ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(single_response));
  }
  flush_settings_.maximum_requests = 10;
  client_ = CreateStreamLayerClient();
  ASSERT_NO_FATAL_FAILURE(maximumRequestsSuccessAssertions(10));
  client_ = CreateStreamLayerClient();
  ASSERT_NO_FATAL_FAILURE(maximumRequestsSuccessAssertions(10, 13));
  client_ = CreateStreamLayerClient();
  ASSERT_NO_FATAL_FAILURE(maximumRequestsSuccessAssertions(10, 9));
  flush_settings_.maximum_requests = 0;
  client_ = CreateStreamLayerClient();
  ASSERT_NO_FATAL_FAILURE(maximumRequestsSuccessAssertions(0, 10));
}
