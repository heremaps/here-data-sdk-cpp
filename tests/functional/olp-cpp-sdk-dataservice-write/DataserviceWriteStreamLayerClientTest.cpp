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
#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/write/StreamLayerClient.h>
#include <olp/dataservice/write/model/PublishDataRequest.h>
#include <olp/dataservice/write/model/PublishSdiiRequest.h>
#include <testutils/CustomParameters.hpp>
#include "Utils.h"

using namespace olp::dataservice::write;
using namespace olp::dataservice::write::model;
using namespace testing;

const std::string kEndpoint = "endpoint";
const std::string kAppid = "dataservice_write_test_appid";
const std::string kSecret = "dataservice_write_test_secret";
const std::string kCatalog = "dataservice_write_test_catalog";
const std::string kLayer = "layer";
const std::string kLayer2 = "layer2";
const std::string kLayerSdii = "layer_sdii";

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
  EXPECT_SUCCESS(result);
  EXPECT_FALSE(result.GetResult().GetTraceID().empty());
}

void PublishSdiiSuccessAssertions(
    const olp::client::ApiResponse<ResponseOk, olp::client::ApiError>& result) {
  EXPECT_SUCCESS(result);
  EXPECT_FALSE(result.GetResult().GetTraceID().GetParentID().empty());
  ASSERT_FALSE(result.GetResult().GetTraceID().GetGeneratedIDs().empty());
  EXPECT_FALSE(result.GetResult().GetTraceID().GetGeneratedIDs().at(0).empty());
}

template <typename T>
void PublishFailureAssertions(
    const olp::client::ApiResponse<T, olp::client::ApiError>& result) {
  EXPECT_FALSE(result.IsSuccessful());
  EXPECT_NE(result.GetError().GetHttpStatusCode(), 200);
  // EXPECT_FALSE(result.GetError().GetMessage().empty());
}

class DataserviceWriteStreamLayerClientTest : public ::testing::Test {
 protected:
  DataserviceWriteStreamLayerClientTest() {
    sdii_data_ = std::make_shared<std::vector<unsigned char>>(
        kSDIITestData, kSDIITestData + kSDIITestDataLength);
  }

  virtual void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(client_ = CreateStreamLayerClient());
    data_ = GenerateData();
  }

  virtual void TearDown() override {
    data_.reset();
    client_.reset();
  }

  std::string GetTestCatalog() {
    return CustomParameters::getArgument(kCatalog);
  }

  std::string GetTestLayer() { return CustomParameters::getArgument(kLayer); }

  std::string GetTestLayer2() { return CustomParameters::getArgument(kLayer2); }

  std::string GetTestLayerSdii() {
    return CustomParameters::getArgument(kLayerSdii);
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

  static void SetUpTestSuite() {
    s_network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();
    s_task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);
  }

  virtual std::shared_ptr<StreamLayerClient> CreateStreamLayerClient() {
    auto network = s_network;

    const auto app_id = CustomParameters::getArgument(kAppid);
    const auto secret = CustomParameters::getArgument(kSecret);

    olp::authentication::Settings authentication_settings({app_id, secret});
    authentication_settings.token_endpoint_url =
        CustomParameters::getArgument(kEndpoint);
    authentication_settings.network_request_handler = network;

    olp::authentication::TokenProviderDefault provider(authentication_settings);

    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;

    olp::client::OlpClientSettings settings;
    settings.authentication_settings = auth_client_settings;
    settings.network_request_handler = network;
    settings.task_scheduler = s_task_scheduler;

    return std::make_shared<StreamLayerClient>(
        olp::client::HRN{GetTestCatalog()}, StreamLayerClientSettings{},
        settings);
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
  static std::shared_ptr<olp::thread::TaskScheduler> s_task_scheduler;

  std::shared_ptr<StreamLayerClient> client_;
  std::shared_ptr<std::vector<unsigned char>> data_;
  std::shared_ptr<std::vector<unsigned char>> sdii_data_;
};

// Static network instance is necessary as it needs to outlive any created
// clients. This is a known limitation as triggered send requests capture the
// network instance inside the callbacks.
std::shared_ptr<olp::http::Network>
    DataserviceWriteStreamLayerClientTest::s_network;

// Static network instance is necessary as it needs to outlive any created
// clients. This is a known limitation as triggered send requests capture the
// task_scheduler instance inside the callbacks, and it could happen that the
// task is trying to destroy task scheduler, that will result in a crash.
std::shared_ptr<olp::thread::TaskScheduler>
    DataserviceWriteStreamLayerClientTest::s_task_scheduler;

TEST_F(DataserviceWriteStreamLayerClientTest, PublishData) {
  auto response =
      client_
          ->PublishData(
              PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()))
          .GetFuture()
          .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_F(DataserviceWriteStreamLayerClientTest, PublishDataGreaterThanTwentyMib) {
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

TEST_F(DataserviceWriteStreamLayerClientTest, PublishDataAsync) {
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

TEST_F(DataserviceWriteStreamLayerClientTest, PublishDataCancel) {
  auto cancel_future = client_->PublishData(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    cancel_future.GetCancellationToken().cancel();
  }).detach();

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

TEST_F(DataserviceWriteStreamLayerClientTest, PublishDataCancelLongDelay) {
  auto cancel_future = client_->PublishData(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    cancel_future.GetCancellationToken().cancel();
  }).detach();

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

TEST_F(DataserviceWriteStreamLayerClientTest,
       PublishDataCancelGetFutureAfterRequestCancelled) {
  auto cancel_future = client_->PublishData(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    cancel_future.GetCancellationToken().cancel();
  }).detach();

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

TEST_F(DataserviceWriteStreamLayerClientTest,
       PublishDataGreaterThanTwentyMibCancel) {
  auto large_data =
      std::make_shared<std::vector<unsigned char>>(kTwentyMib + 1, 'z');

  auto cancel_future = client_->PublishData(
      PublishDataRequest().WithData(large_data).WithLayerId(GetTestLayer()));

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cancel_future.GetCancellationToken().cancel();
  }).detach();

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

TEST_F(DataserviceWriteStreamLayerClientTest, IncorrectLayer) {
  auto response =
      client_
          ->PublishData(
              PublishDataRequest().WithData(data_).WithLayerId("BadLayer"))
          .GetFuture()
          .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_F(DataserviceWriteStreamLayerClientTest, NullData) {
  auto response =
      client_
          ->PublishData(PublishDataRequest().WithData(nullptr).WithLayerId(
              GetTestLayer()))
          .GetFuture()
          .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_F(DataserviceWriteStreamLayerClientTest, CustomTraceId) {
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

TEST_F(DataserviceWriteStreamLayerClientTest, BillingTag) {
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
TEST_F(DataserviceWriteStreamLayerClientTest, ChecksumValid) {
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

TEST_F(DataserviceWriteStreamLayerClientTest, ChecksumGarbageString) {
  auto response = client_
                      ->PublishData(PublishDataRequest()
                                        .WithData(data_)
                                        .WithLayerId(GetTestLayer())
                                        .WithChecksum("GarbageChecksum"))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_F(DataserviceWriteStreamLayerClientTest, SequentialPublishSameLayer) {
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

TEST_F(DataserviceWriteStreamLayerClientTest, SequentialPublishDifferentLayer) {
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

TEST_F(DataserviceWriteStreamLayerClientTest, ConcurrentPublishSameIngestApi) {
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

TEST_F(DataserviceWriteStreamLayerClientTest,
       ConcurrentPublishDifferentIngestApi) {
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

TEST_F(DataserviceWriteStreamLayerClientTest, PublishSdii) {
  auto response = client_
                      ->PublishSdii(PublishSdiiRequest()
                                        .WithSdiiMessageList(sdii_data_)
                                        .WithLayerId(GetTestLayerSdii()))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishSdiiSuccessAssertions(response));
}

TEST_F(DataserviceWriteStreamLayerClientTest, PublishSdiiAsync) {
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

TEST_F(DataserviceWriteStreamLayerClientTest, PublishSdiiCancel) {
  auto cancel_future =
      client_->PublishSdii(PublishSdiiRequest()
                               .WithSdiiMessageList(sdii_data_)
                               .WithLayerId(GetTestLayerSdii()));

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    cancel_future.GetCancellationToken().cancel();
  }).detach();

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

TEST_F(DataserviceWriteStreamLayerClientTest, PublishSdiiCancelLongDelay) {
  auto cancel_future =
      client_->PublishSdii(PublishSdiiRequest()
                               .WithSdiiMessageList(sdii_data_)
                               .WithLayerId(GetTestLayerSdii()));

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    cancel_future.GetCancellationToken().cancel();
  }).detach();

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

TEST_F(DataserviceWriteStreamLayerClientTest, PublishSDIINonSDIIData) {
  auto response =
      client_
          ->PublishSdii(
              PublishSdiiRequest().WithSdiiMessageList(data_).WithLayerId(
                  GetTestLayerSdii()))
          .GetFuture()
          .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_F(DataserviceWriteStreamLayerClientTest, PublishSDIIIncorrectLayer) {
  auto response = client_
                      ->PublishSdii(PublishSdiiRequest()
                                        .WithSdiiMessageList(sdii_data_)
                                        .WithLayerId("BadLayer"))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_F(DataserviceWriteStreamLayerClientTest, PublishSDIICustomTraceId) {
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

TEST_F(DataserviceWriteStreamLayerClientTest, PublishSDIIBillingTag) {
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
TEST_F(DataserviceWriteStreamLayerClientTest, SDIIChecksumValid) {
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

TEST_F(DataserviceWriteStreamLayerClientTest, SDIIChecksumGarbageString) {
  auto response = client_
                      ->PublishSdii(PublishSdiiRequest()
                                        .WithSdiiMessageList(sdii_data_)
                                        .WithLayerId(GetTestLayerSdii())
                                        .WithChecksum("GarbageChecksum"))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response));
}

TEST_F(DataserviceWriteStreamLayerClientTest,
       SDIIConcurrentPublishSameIngestApi) {
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

}  // namespace
