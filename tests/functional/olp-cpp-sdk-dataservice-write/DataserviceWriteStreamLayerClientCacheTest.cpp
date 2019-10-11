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
#include "testables/FlushEventListenerTestable.h"

using namespace olp::dataservice::write;
using namespace olp::dataservice::write::model;
using namespace olp::tests::common;
using namespace testing;

const std::string kEndpoint = "endpoint";
const std::string kAppid = "dataservice_write_test_appid";
const std::string kSecret = "dataservice_write_test_secret";
const std::string kCatalog = "dataservice_write_test_catalog";
const std::string kLayer = "layer";

namespace {

const std::string kBillingTag = "OlpCppSdkTest";

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

template <typename T>
void PublishFailureAssertions(
    const olp::client::ApiResponse<T, olp::client::ApiError>& result) {
  EXPECT_FALSE(result.IsSuccessful());
  EXPECT_NE(result.GetError().GetHttpStatusCode(), 200);
  // EXPECT_FALSE(result.GetError().GetMessage().empty());
}

class DataserviceWriteStreamLayerClientCacheTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    s_network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();
  }

  virtual void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(client_ = CreateStreamLayerClient());
    data_ = GenerateData();
  }

  virtual void TearDown() override {
    data_.reset();
    client_.reset();
    if (disk_cache_) {
      disk_cache_->Close();
    }
  }

  std::string GetTestCatalog() {
    return CustomParameters::getArgument(kCatalog);
  }

  std::string GetTestLayer() { return CustomParameters::getArgument(kLayer); }

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
    settings.task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);

    disk_cache_ = std::make_shared<olp::cache::DefaultCache>();
    EXPECT_EQ(disk_cache_->Open(),
              olp::cache::DefaultCache::StorageOpenResult::Success);
    settings.cache = disk_cache_;

    return std::make_shared<StreamLayerClient>(
        olp::client::HRN{GetTestCatalog()}, settings, flush_settings_);
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

  std::shared_ptr<StreamLayerClient> client_;
  std::shared_ptr<std::vector<unsigned char>> data_;

  std::shared_ptr<olp::cache::DefaultCache> disk_cache_;
  FlushSettings flush_settings_;
};

// Static network instance is necessary as it needs to outlive any created
// clients. This is a known limitation as triggered send requests capture the
// network instance inside the callbacks.
std::shared_ptr<olp::http::Network>
    DataserviceWriteStreamLayerClientCacheTest::s_network;

TEST_F(DataserviceWriteStreamLayerClientCacheTest, Queue) {
  auto error = client_->Queue(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  ASSERT_FALSE(error) << error.get();
}

TEST_F(DataserviceWriteStreamLayerClientCacheTest, QueueNullData) {
  auto error = client_->Queue(
      PublishDataRequest().WithData(nullptr).WithLayerId(GetTestLayer()));

  ASSERT_TRUE(error);
}

TEST_F(DataserviceWriteStreamLayerClientCacheTest, QueueExtraRequestParams) {
  const auto uuid = GenerateRandomUUID();

  auto error = client_->Queue(PublishDataRequest()
                                  .WithData(data_)
                                  .WithLayerId(GetTestLayer())
                                  .WithTraceId(uuid)
                                  .WithBillingTag(kBillingTag));

  ASSERT_FALSE(error) << error.get();
}

#ifdef DATASERVICE_WRITE_HAS_OPENSSL
TEST_F(DataserviceWriteStreamLayerClientCacheTest, QueueWithChecksum) {
  const std::string data_string(data_->begin(), data_->end());
  auto checksum = sha256(data_string);

  auto error = client_->Queue(PublishDataRequest()
                                  .WithData(data_)
                                  .WithLayerId(GetTestLayer())
                                  .WithChecksum(checksum));

  ASSERT_FALSE(error) << error.get();
}
#endif

TEST_F(DataserviceWriteStreamLayerClientCacheTest, FlushDataSingle) {
  auto error = client_->Queue(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  ASSERT_FALSE(error) << error.get();

  auto response = client_->Flush().GetFuture().get();

  ASSERT_FALSE(response.empty());
  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response[0]));
}

TEST_F(DataserviceWriteStreamLayerClientCacheTest, FlushDataMultiple) {
  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(5));

  auto response = client_->Flush().GetFuture().get();

  ASSERT_EQ(5, response.size());
  for (auto& single_response : response) {
    ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(single_response));
  }
}

TEST_F(DataserviceWriteStreamLayerClientCacheTest, FlushDataSingleAsync) {
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

TEST_F(DataserviceWriteStreamLayerClientCacheTest, FlushDataMultipleAsync) {
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

TEST_F(DataserviceWriteStreamLayerClientCacheTest, FlushDataCancel) {
  auto error = client_->Queue(
      PublishDataRequest().WithData(data_).WithLayerId(GetTestLayer()));

  ASSERT_FALSE(error) << error.get();

  auto cancel_future = client_->Flush();

  std::thread([cancel_future]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    cancel_future.GetCancellationToken().cancel();
  }).detach();

  auto response = cancel_future.GetFuture().get();

  ASSERT_EQ(1, response.size());
  if (response[0].IsSuccessful()) {
    SUCCEED();
    return;
  }

  ASSERT_NO_FATAL_FAILURE(PublishFailureAssertions(response[0]));
}

TEST_F(DataserviceWriteStreamLayerClientCacheTest,
       DISABLED_FlushListenerMetrics) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 3;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(3));

  auto default_listener = std::make_shared<FlushEventListenerTestable>();

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

TEST_F(DataserviceWriteStreamLayerClientCacheTest,
       DISABLED_FlushListenerMetricsSetListenerBeforeQueuing) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 3;
  client_ = CreateStreamLayerClient();

  auto default_listener = std::make_shared<FlushEventListenerTestable>();

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

TEST_F(DataserviceWriteStreamLayerClientCacheTest,
       DISABLED_FlushListenerDisable) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 3;
  client_ = CreateStreamLayerClient();

  auto default_listener = std::make_shared<FlushEventListenerTestable>();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(3));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // TODO: uncomment this code once the auto-flush mechanism will be turned.
  //  auto disable_future = client_->Disable();
  //  auto status = disable_future.wait_for(std::chrono::seconds(5));
  //  if (status != std::future_status::ready) {
  //    FAIL() << "Timeout waiting for auto flushing to be disabled";
  //  }
  //  disable_future.get();

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsFailed());
}

TEST_F(DataserviceWriteStreamLayerClientCacheTest,
       DISABLED_FlushListenerMetricsMultipleFlushEventsInSeries) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 2;
  client_ = CreateStreamLayerClient();

  auto default_listener = std::make_shared<FlushEventListenerTestable>();

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

TEST_F(DataserviceWriteStreamLayerClientCacheTest,
       DISABLED_FlushListenerMetricsMultipleFlushEventsInParallel) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 2;
  flush_settings_.events_per_single_flush =
      flush_settings_.auto_flush_num_events;
  client_ = CreateStreamLayerClient();

  auto default_listener = std::make_shared<FlushEventListenerTestable>();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(6));

  for (int i = 0; default_listener->GetNumFlushedRequests() < 6; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (i > 500) {
      FAIL() << "Timeout waiting for Flush Event Listener Results";
    }
  }

  EXPECT_LE(3, default_listener->GetNumFlushEvents());
  EXPECT_LE(3, default_listener->GetNumFlushEventsAttempted());
  // TODO: Investigate why there are more triggeres in auto flushing then
  // requests. Seems AutoFlushController is trying to flush to often.
  // EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(6, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_F(
    DataserviceWriteStreamLayerClientCacheTest,
    DISABLED_FlushListenerMetricsMultipleFlushEventsInParallelStaggeredQueue) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 2;
  flush_settings_.events_per_single_flush =
      flush_settings_.auto_flush_num_events;
  client_ = CreateStreamLayerClient();

  auto default_listener = std::make_shared<FlushEventListenerTestable>();

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
  // TODO: Investigate why there are more triggeres in auto flushing then
  // requests. Seems AutoFlushController is trying to flush to often.
  // EXPECT_EQ(0, default_listener->GetNumFlushEventsFailed());
  EXPECT_EQ(10, default_listener->GetNumFlushedRequests());
  EXPECT_EQ(0, default_listener->GetNumFlushedRequestsFailed());
}

TEST_F(DataserviceWriteStreamLayerClientCacheTest,
       DISABLED_FlushListenerNotifications) {
  disk_cache_->Close();
  flush_settings_.auto_flush_num_events = 3;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(3));

  class NotificationListener
      : public FlushEventListener<const StreamLayerClient::FlushResponse&> {
   public:
    void NotifyFlushEventStarted() override { events_started_++; }

    void NotifyFlushEventResults(
        const StreamLayerClient::FlushResponse& results) override {
      std::lock_guard<std::mutex> lock(results_mutex_);
      results_ = std::move(results);
    }

    void NotifyFlushMetricsHasChanged(FlushMetrics metrics) override {
      // no-op
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

TEST_F(DataserviceWriteStreamLayerClientCacheTest,
       DISABLED_FlushSettingsAutoFlushInterval) {
  disk_cache_->Close();
  flush_settings_.auto_flush_interval = 10;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));

  auto default_listener = std::make_shared<FlushEventListenerTestable>();

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

TEST_F(DataserviceWriteStreamLayerClientCacheTest,
       DISABLED_FlushSettingsAutoFlushIntervalDisable) {
  disk_cache_->Close();
  flush_settings_.auto_flush_interval = 2;
  client_ = CreateStreamLayerClient();

  ASSERT_NO_FATAL_FAILURE(QueueMultipleEvents(2));

  auto default_listener = std::make_shared<FlushEventListenerTestable>();

  std::this_thread::sleep_for(std::chrono::milliseconds(2100));

  // TODO: uncomment this code once the auto-flush mechanism will be turned.
  //  auto disable_future = client_->Disable();
  //  auto status = disable_future.wait_for(std::chrono::seconds(5));
  //  if (status != std::future_status::ready) {
  //    FAIL() << "Timeout waiting for auto flushing to be disabled";
  //  }
  //  disable_future.get();

  EXPECT_EQ(1, default_listener->GetNumFlushEvents());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsAttempted());
  EXPECT_EQ(1, default_listener->GetNumFlushEventsFailed());
}

}  // namespace
