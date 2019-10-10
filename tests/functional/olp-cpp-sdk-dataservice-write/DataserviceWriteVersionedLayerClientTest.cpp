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
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/write/VersionedLayerClient.h>
#include <olp/dataservice/write/model/StartBatchRequest.h>
#include <testutils/CustomParameters.hpp>

namespace {

using namespace olp::dataservice::write;
using namespace olp::dataservice::write::model;

const std::string kEndpoint = "endpoint";
const std::string kAppid = "dataservice_write_test_appid";
const std::string kSecret = "dataservice_write_test_secret";
const std::string kCatalog = "dataservice_write_test_catalog";
const std::string kLayer = "layer";
const std::string kLayer2 = "layer2";
const std::string kLayerSdii = "layer_sdii";
const std::string kVersionedLayer = "versioned_layer";

class DataserviceWriteVersionedLayerClientTest : public ::testing::Test {
 protected:
  static std::shared_ptr<olp::http::Network> s_network;

  static void SetUpTestSuite() {
    s_network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();
  }

  virtual void SetUp() override { client_ = CreateVersionedLayerClient(); }

  virtual void TearDown() override { client_ = nullptr; }

  std::shared_ptr<VersionedLayerClient> CreateVersionedLayerClient() {
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

    return std::make_shared<VersionedLayerClient>(
        olp::client::HRN{CustomParameters::getArgument(kCatalog)}, settings);
  }

  std::shared_ptr<VersionedLayerClient> client_;
};

// Static network instance is necessary as it needs to outlive any created
// clients. This is a known limitation as triggered send requests capture the
// network instance inside the callbacks.
std::shared_ptr<olp::http::Network>
    DataserviceWriteVersionedLayerClientTest::s_network;

TEST_F(DataserviceWriteVersionedLayerClientTest, StartBatchInvalid) {
  auto versioned_client = CreateVersionedLayerClient();
  auto response =
      versioned_client->StartBatch(StartBatchRequest()).GetFuture().get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_FALSE(response.GetResult().GetId());
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            response.GetError().GetErrorCode());

  auto get_batch_response =
      versioned_client->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_FALSE(get_batch_response.IsSuccessful());

  auto complete_batch_response =
      versioned_client->CompleteBatch(get_batch_response.GetResult())
          .GetFuture()
          .get();
  ASSERT_FALSE(complete_batch_response.IsSuccessful());

  auto cancel_batch_response =
      versioned_client->CancelBatch(get_batch_response.GetResult())
          .GetFuture()
          .get();
  ASSERT_FALSE(cancel_batch_response.IsSuccessful());
}

TEST_F(DataserviceWriteVersionedLayerClientTest, StartBatch) {
  auto versioned_client = CreateVersionedLayerClient();
  auto response = versioned_client
                      ->StartBatch(StartBatchRequest().WithLayers(
                          {CustomParameters::getArgument(kVersionedLayer)}))
                      .GetFuture()
                      .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto get_batch_response =
      versioned_client->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(get_batch_response.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            get_batch_response.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            get_batch_response.GetResult().GetDetails()->GetState());

  auto complete_batch_response =
      versioned_client->CompleteBatch(get_batch_response.GetResult())
          .GetFuture()
          .get();
  ASSERT_TRUE(complete_batch_response.IsSuccessful());

  get_batch_response =
      versioned_client->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(get_batch_response.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            get_batch_response.GetResult().GetId().value());
  ASSERT_EQ("submitted",
            get_batch_response.GetResult().GetDetails()->GetState());

  for (int i = 0; i < 100; ++i) {
    get_batch_response =
        versioned_client->GetBatch(response.GetResult()).GetFuture().get();

    ASSERT_TRUE(get_batch_response.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetId().value(),
              get_batch_response.GetResult().GetId().value());
    if (get_batch_response.GetResult().GetDetails()->GetState() !=
        "succeeded") {
      ASSERT_EQ("submitted",
                get_batch_response.GetResult().GetDetails()->GetState());
    } else {
      break;
    }
  }
  // There are can be a case that GetBatch() is not "succeeded", but in
  // "submitted" state even after 100 iterations,
  // which actually means that there are might be a problem on the server side,
  // (or just long delay). Thus, better to rewrite this test, or do not rely on
  // the real server, but use mocked server.
  // ASSERT_EQ("succeeded",
  // get_batch_response.GetResult().GetDetails()->GetState());
}

TEST_F(DataserviceWriteVersionedLayerClientTest, DeleteClient) {
  auto versioned_client = CreateVersionedLayerClient();
  auto fut = versioned_client
                 ->StartBatch(StartBatchRequest().WithLayers(
                     {CustomParameters::getArgument(kVersionedLayer)}))
                 .GetFuture();
  versioned_client = nullptr;

  auto response = fut.get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto cancel_batch_response =
      client_->CancelBatch(response.GetResult()).GetFuture().get();
  ASSERT_TRUE(cancel_batch_response.IsSuccessful());

  auto get_batch_response =
      client_->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(get_batch_response.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            get_batch_response.GetResult().GetId().value());
  ASSERT_EQ("cancelled",
            get_batch_response.GetResult().GetDetails()->GetState());
}

TEST_F(DataserviceWriteVersionedLayerClientTest, GetBaseVersion) {
  auto versioned_client = CreateVersionedLayerClient();
  auto response = versioned_client->GetBaseVersion().GetFuture().get();

  ASSERT_TRUE(response.IsSuccessful());
  auto version_response = response.GetResult();
  ASSERT_GE(version_response.GetVersion(), 0);
}

TEST_F(DataserviceWriteVersionedLayerClientTest, CancelBatch) {
  auto versioned_client = CreateVersionedLayerClient();
  auto response = versioned_client
                      ->StartBatch(StartBatchRequest().WithLayers(
                          {CustomParameters::getArgument(kVersionedLayer)}))
                      .GetFuture()
                      .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto get_batch_response =
      versioned_client->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(get_batch_response.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            get_batch_response.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            get_batch_response.GetResult().GetDetails()->GetState());

  auto cancel_batch_response =
      versioned_client->CancelBatch(get_batch_response.GetResult())
          .GetFuture()
          .get();
  ASSERT_TRUE(cancel_batch_response.IsSuccessful());

  get_batch_response =
      versioned_client->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(get_batch_response.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            get_batch_response.GetResult().GetId().value());
  ASSERT_EQ("cancelled",
            get_batch_response.GetResult().GetDetails()->GetState());
}

TEST_F(DataserviceWriteVersionedLayerClientTest, CancelAllBatch) {
  auto versioned_client = CreateVersionedLayerClient();
  auto response_future =
      versioned_client
          ->StartBatch(StartBatchRequest().WithLayers(
              {CustomParameters::getArgument(kVersionedLayer)}))
          .GetFuture();

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  versioned_client->CancelAll();

  auto response = response_future.get();
  ASSERT_FALSE(response.IsSuccessful());
}

TEST_F(DataserviceWriteVersionedLayerClientTest, PublishToBatch) {
  auto versioned_client = CreateVersionedLayerClient();
  auto response = versioned_client
                      ->StartBatch(StartBatchRequest().WithLayers(
                          {CustomParameters::getArgument(kVersionedLayer)}))
                      .GetFuture()
                      .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto get_batch_response =
      versioned_client->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(get_batch_response.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            get_batch_response.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            get_batch_response.GetResult().GetDetails()->GetState());

  auto publish_to_batch_response =
      versioned_client
          ->PublishToBatch(
              response.GetResult(),
              PublishPartitionDataRequest()
                  .WithData(
                      std::make_shared<std::vector<unsigned char>>(20, 0x30))
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithPartitionId("1111"))
          .GetFuture()
          .get();

  ASSERT_TRUE(publish_to_batch_response.IsSuccessful());
  ASSERT_EQ("1111", publish_to_batch_response.GetResult().GetTraceID());

  auto complete_batch_response =
      versioned_client->CompleteBatch(get_batch_response.GetResult())
          .GetFuture()
          .get();

  ASSERT_TRUE(complete_batch_response.IsSuccessful());

  for (int i = 0; i < 100; ++i) {
    get_batch_response =
        versioned_client->GetBatch(response.GetResult()).GetFuture().get();

    ASSERT_TRUE(get_batch_response.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetId().value(),
              get_batch_response.GetResult().GetId().value());
    if (get_batch_response.GetResult().GetDetails()->GetState() !=
        "succeeded") {
      ASSERT_EQ("submitted",
                get_batch_response.GetResult().GetDetails()->GetState());
    } else {
      break;
    }
  }
  // There are can be a case that GetBatch() is not "succeeded", but in
  // "submitted" state even after 100 iterations,
  // which actually means that there are might be a problem on the server side,
  // (or just long delay). Thus, better to rewrite this test, or do not rely on
  // the real server, but use mocked server.
  // ASSERT_EQ("succeeded",
  // get_batch_response.GetResult().GetDetails()->GetState());
}

TEST_F(DataserviceWriteVersionedLayerClientTest, PublishToBatchDeleteClient) {
  auto versioned_client = CreateVersionedLayerClient();
  auto response = versioned_client
                      ->StartBatch(StartBatchRequest().WithLayers(
                          {CustomParameters::getArgument(kVersionedLayer)}))
                      .GetFuture()
                      .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto get_batch_response =
      versioned_client->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(get_batch_response.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            get_batch_response.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            get_batch_response.GetResult().GetDetails()->GetState());

  auto publish_to_batch_future =
      versioned_client
          ->PublishToBatch(
              response.GetResult(),
              PublishPartitionDataRequest()
                  .WithData(
                      std::make_shared<std::vector<unsigned char>>(20, 0x30))
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithPartitionId("1111"))
          .GetFuture();

  auto publish_to_batch_future2 =
      versioned_client
          ->PublishToBatch(
              response.GetResult(),
              PublishPartitionDataRequest()
                  .WithData(
                      std::make_shared<std::vector<unsigned char>>(20, 0x31))
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithPartitionId("1112"))
          .GetFuture();

  versioned_client = nullptr;

  auto publish_to_batch_response = publish_to_batch_future.get();
  auto publish_to_batch_response2 = publish_to_batch_future2.get();

  ASSERT_TRUE(publish_to_batch_response.IsSuccessful());
  ASSERT_EQ("1111", publish_to_batch_response.GetResult().GetTraceID());
  ASSERT_TRUE(publish_to_batch_response2.IsSuccessful());
  ASSERT_EQ("1112", publish_to_batch_response2.GetResult().GetTraceID());

  versioned_client = CreateVersionedLayerClient();

  auto complete_batch_response =
      versioned_client->CompleteBatch(get_batch_response.GetResult())
          .GetFuture()
          .get();

  ASSERT_TRUE(complete_batch_response.IsSuccessful());

  for (int i = 0; i < 100; ++i) {
    get_batch_response =
        versioned_client->GetBatch(response.GetResult()).GetFuture().get();

    ASSERT_TRUE(get_batch_response.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetId().value(),
              get_batch_response.GetResult().GetId().value());
    if (get_batch_response.GetResult().GetDetails()->GetState() !=
        "succeeded") {
      ASSERT_EQ("submitted",
                get_batch_response.GetResult().GetDetails()->GetState());
    } else {
      break;
    }
  }
  // This check contradicts with the previous assertion - we can have a case,
  // when the state of the last (i == 99) get_batch_response is "submitted',
  // but not 'succeeded', thus, in the previous loop we accept such case
  // as a valid one, but here, we treat such case as an error.
  // ASSERT_EQ("succeeded",
  // get_batch_response.GetResult().GetDetails()->GetState());
}

TEST_F(DataserviceWriteVersionedLayerClientTest, PublishToBatchMulti) {
  auto versioned_client = CreateVersionedLayerClient();
  auto response = versioned_client
                      ->StartBatch(StartBatchRequest().WithLayers(
                          {CustomParameters::getArgument(kVersionedLayer)}))
                      .GetFuture()
                      .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto get_batch_response =
      versioned_client->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(get_batch_response.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            get_batch_response.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            get_batch_response.GetResult().GetDetails()->GetState());

  auto publish_to_batch_future =
      versioned_client
          ->PublishToBatch(
              response.GetResult(),
              PublishPartitionDataRequest()
                  .WithData(
                      std::make_shared<std::vector<unsigned char>>(20, 0x30))
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithPartitionId("1111"))
          .GetFuture();

  auto publish_to_batch_future2 =
      versioned_client
          ->PublishToBatch(
              response.GetResult(),
              PublishPartitionDataRequest()
                  .WithData(
                      std::make_shared<std::vector<unsigned char>>(20, 0x31))
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithPartitionId("1112"))
          .GetFuture();

  auto publish_to_batch_response = publish_to_batch_future.get();
  auto publish_to_batch_response2 = publish_to_batch_future2.get();

  ASSERT_TRUE(publish_to_batch_response.IsSuccessful());
  ASSERT_EQ("1111", publish_to_batch_response.GetResult().GetTraceID());
  ASSERT_TRUE(publish_to_batch_response2.IsSuccessful());
  ASSERT_EQ("1112", publish_to_batch_response2.GetResult().GetTraceID());

  auto complete_batch_response =
      versioned_client->CompleteBatch(get_batch_response.GetResult())
          .GetFuture()
          .get();

  ASSERT_TRUE(complete_batch_response.IsSuccessful());

  for (int i = 0; i < 100; ++i) {
    get_batch_response =
        versioned_client->GetBatch(response.GetResult()).GetFuture().get();

    ASSERT_TRUE(get_batch_response.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetId().value(),
              get_batch_response.GetResult().GetId().value());
    if (get_batch_response.GetResult().GetDetails()->GetState() !=
        "succeeded") {
      ASSERT_EQ("submitted",
                get_batch_response.GetResult().GetDetails()->GetState());
    } else {
      break;
    }
  }
  // There are can be a case that GetBatch() is not "succeeded", but in
  // "submitted" state even after 100 iterations,
  // which actually means that there are might be a problem on the server side,
  // (or just long delay). Thus, better to rewrite this test, or do not rely on
  // the real server, but use mocked server.
  // ASSERT_EQ("succeeded",
  // get_batch_response.GetResult().GetDetails()->GetState());
}

TEST_F(DataserviceWriteVersionedLayerClientTest, PublishToBatchCancel) {
  auto versioned_client = CreateVersionedLayerClient();
  auto response = versioned_client
                      ->StartBatch(StartBatchRequest().WithLayers(
                          {CustomParameters::getArgument(kVersionedLayer)}))
                      .GetFuture()
                      .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto get_batch_response =
      versioned_client->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(get_batch_response.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            get_batch_response.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            get_batch_response.GetResult().GetDetails()->GetState());

  auto publish_to_batch_future =
      versioned_client
          ->PublishToBatch(
              response.GetResult(),
              PublishPartitionDataRequest()
                  .WithData(
                      std::make_shared<std::vector<unsigned char>>(20, 0x30))
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithPartitionId("1111"))
          .GetFuture();

  versioned_client->CancelAll();

  auto publish_to_batch_response = publish_to_batch_future.get();
  ASSERT_FALSE(publish_to_batch_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            publish_to_batch_response.GetError().GetErrorCode());

  auto cancel_batch_response =
      versioned_client->CancelBatch(get_batch_response.GetResult())
          .GetFuture()
          .get();
  ASSERT_TRUE(cancel_batch_response.IsSuccessful());

  get_batch_response =
      versioned_client->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(get_batch_response.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            get_batch_response.GetResult().GetId().value());
  ASSERT_EQ("cancelled",
            get_batch_response.GetResult().GetDetails()->GetState());
}

TEST_F(DataserviceWriteVersionedLayerClientTest, CheckDataExists) {
  auto versioned_client = CreateVersionedLayerClient();
  auto fut =
      versioned_client
          ->CheckDataExists(
              CheckDataExistsRequest()
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithDataHandle("5d2082c3-9738-4de7-bde0-4a52527dab37"))
          .GetFuture();
  versioned_client = nullptr;

  auto response = fut.get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_EQ(response.GetResult(), 200);
}

TEST_F(DataserviceWriteVersionedLayerClientTest, CheckDataNotExists) {
  auto versioned_client = CreateVersionedLayerClient();
  auto fut =
      versioned_client
          ->CheckDataExists(
              CheckDataExistsRequest()
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithDataHandle("5d2082c3-9738-4de7-bde0-4a52527dab34"))
          .GetFuture();
  versioned_client = nullptr;

  auto response = fut.get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_EQ(response.GetResult(), 404);
}

}  // namespace
