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

#include <olp/dataservice/write/VersionedLayerClient.h>

#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/HRN.h>

#include <olp/dataservice/write/model/StartBatchRequest.h>

#include "testutils/CustomParameters.hpp"

using namespace olp::dataservice::write;
using namespace olp::dataservice::write::model;

const std::string kEndpoint = "endpoint";
const std::string kAppid = "appid";
const std::string kSecret = "secret";
const std::string kCatalog = "catalog";
const std::string kLayer = "layer";
const std::string kLayer2 = "layer2";
const std::string kLayerSdii = "layer_sdii";
const std::string kVersionedLayer = "versioned_layer";

class VersionedLayerClientTest : public ::testing::TestWithParam<bool> {};

class VersionedLayerClientOfflineTest : public VersionedLayerClientTest {};

INSTANTIATE_TEST_SUITE_P(TestOffline, VersionedLayerClientOfflineTest,
                         ::testing::Values(false));

TEST_P(VersionedLayerClientOfflineTest, StartBatchRequestTest) {
  ASSERT_FALSE(StartBatchRequest().GetLayers());
  ASSERT_FALSE(StartBatchRequest().GetVersionDependencies());
  ASSERT_FALSE(StartBatchRequest().GetBillingTag());

  auto sbr =
      StartBatchRequest()
          .WithLayers({"layer1", "layer2"})
          .WithVersionDependencies({{false, "hrn1", 0}, {true, "hrn2", 1}})
          .WithBillingTag("billingTag");
  ASSERT_TRUE(sbr.GetLayers());
  ASSERT_FALSE(sbr.GetLayers()->empty());
  ASSERT_EQ(2ull, sbr.GetLayers()->size());
  ASSERT_EQ("layer1", (*sbr.GetLayers())[0]);
  ASSERT_EQ("layer2", (*sbr.GetLayers())[1]);
  ASSERT_TRUE(sbr.GetVersionDependencies());
  ASSERT_FALSE(sbr.GetVersionDependencies()->empty());
  ASSERT_EQ(2ull, sbr.GetVersionDependencies()->size());
  ASSERT_FALSE((*sbr.GetVersionDependencies())[0].GetDirect());
  ASSERT_TRUE((*sbr.GetVersionDependencies())[1].GetDirect());
  ASSERT_EQ("hrn1", (*sbr.GetVersionDependencies())[0].GetHrn());
  ASSERT_EQ("hrn2", (*sbr.GetVersionDependencies())[1].GetHrn());
  ASSERT_EQ(0, (*sbr.GetVersionDependencies())[0].GetVersion());
  ASSERT_EQ(1, (*sbr.GetVersionDependencies())[1].GetVersion());
  ASSERT_TRUE(sbr.GetBillingTag());
  ASSERT_EQ("billingTag", (*sbr.GetBillingTag()));
}

class VersionedLayerClientOnlineTest : public VersionedLayerClientTest {
 public:
 protected:
  virtual void SetUp() override { client_ = createVersionedLayerClient(); }

  virtual void TearDown() override { client_ = nullptr; }

  std::shared_ptr<VersionedLayerClient> createVersionedLayerClient() {
    olp::authentication::Settings settings;
    settings.token_endpoint_url = CustomParameters::getArgument(kEndpoint);

    olp::client::OlpClientSettings clientSettings;
    clientSettings.authentication_settings =
        olp::client::AuthenticationSettings{
            olp::authentication::TokenProviderDefault{
                CustomParameters::getArgument(kAppid),
                CustomParameters::getArgument(kSecret), settings}};
    return std::make_shared<VersionedLayerClient>(
        olp::client::HRN{CustomParameters::getArgument(kCatalog)},
        clientSettings);
  }

  std::shared_ptr<VersionedLayerClient> client_;
};

INSTANTIATE_TEST_SUITE_P(TestOnline, VersionedLayerClientOnlineTest,
                         ::testing::Values(true));

TEST_P(VersionedLayerClientOnlineTest, StartBatchInvalidTest) {
  auto versionedClient = createVersionedLayerClient();
  auto response =
      versionedClient->StartBatch(StartBatchRequest()).GetFuture().get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_FALSE(response.GetResult().GetId());
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            response.GetError().GetErrorCode());

  auto getBatchResponse =
      versionedClient->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_FALSE(getBatchResponse.IsSuccessful());

  auto completeBatchResponse =
      versionedClient->CompleteBatch(getBatchResponse.GetResult())
          .GetFuture()
          .get();
  ASSERT_FALSE(completeBatchResponse.IsSuccessful());

  auto cancelBatchResponse =
      versionedClient->CancelBatch(getBatchResponse.GetResult())
          .GetFuture()
          .get();
  ASSERT_FALSE(cancelBatchResponse.IsSuccessful());
}

TEST_P(VersionedLayerClientOnlineTest, StartBatchTest) {
  auto versionedClient = createVersionedLayerClient();
  auto response = versionedClient
                      ->StartBatch(StartBatchRequest().WithLayers(
                          {CustomParameters::getArgument(kVersionedLayer)}))
                      .GetFuture()
                      .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto getBatchResponse =
      versionedClient->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(getBatchResponse.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            getBatchResponse.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            getBatchResponse.GetResult().GetDetails()->GetState());

  auto completeBatchResponse =
      versionedClient->CompleteBatch(getBatchResponse.GetResult())
          .GetFuture()
          .get();
  ASSERT_TRUE(completeBatchResponse.IsSuccessful());

  getBatchResponse =
      versionedClient->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(getBatchResponse.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            getBatchResponse.GetResult().GetId().value());
  ASSERT_EQ("submitted", getBatchResponse.GetResult().GetDetails()->GetState());

  for (int i = 0; i < 100; ++i) {
    getBatchResponse =
        versionedClient->GetBatch(response.GetResult()).GetFuture().get();

    ASSERT_TRUE(getBatchResponse.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetId().value(),
              getBatchResponse.GetResult().GetId().value());
    if (getBatchResponse.GetResult().GetDetails()->GetState() != "succeeded") {
      ASSERT_EQ("submitted",
                getBatchResponse.GetResult().GetDetails()->GetState());
    } else {
      break;
    }
  }
  ASSERT_EQ("succeeded", getBatchResponse.GetResult().GetDetails()->GetState());
}

TEST_P(VersionedLayerClientOnlineTest, DeleteClientTest) {
  auto versionedClient = createVersionedLayerClient();
  auto fut = versionedClient
                 ->StartBatch(StartBatchRequest().WithLayers(
                     {CustomParameters::getArgument(kVersionedLayer)}))
                 .GetFuture();
  versionedClient = nullptr;

  auto response = fut.get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto cancelBatchResponse =
      client_->CancelBatch(response.GetResult()).GetFuture().get();
  ASSERT_TRUE(cancelBatchResponse.IsSuccessful());

  auto getBatchResponse =
      client_->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(getBatchResponse.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            getBatchResponse.GetResult().GetId().value());
  ASSERT_EQ("cancelled", getBatchResponse.GetResult().GetDetails()->GetState());
}

TEST_P(VersionedLayerClientOnlineTest, GetBaseVersionTest) {
  auto versionedClient = createVersionedLayerClient();
  auto response = versionedClient->GetBaseVersion().GetFuture().get();

  ASSERT_TRUE(response.IsSuccessful());
  auto versionResponse = response.GetResult();
  ASSERT_GE(versionResponse.GetVersion(), 0);
}

TEST_P(VersionedLayerClientOnlineTest, CancelBatchTest) {
  auto versionedClient = createVersionedLayerClient();
  auto response = versionedClient
                      ->StartBatch(StartBatchRequest().WithLayers(
                          {CustomParameters::getArgument(kVersionedLayer)}))
                      .GetFuture()
                      .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto getBatchResponse =
      versionedClient->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(getBatchResponse.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            getBatchResponse.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            getBatchResponse.GetResult().GetDetails()->GetState());

  auto cancelBatchResponse =
      versionedClient->CancelBatch(getBatchResponse.GetResult())
          .GetFuture()
          .get();
  ASSERT_TRUE(cancelBatchResponse.IsSuccessful());

  getBatchResponse =
      versionedClient->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(getBatchResponse.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            getBatchResponse.GetResult().GetId().value());
  ASSERT_EQ("cancelled", getBatchResponse.GetResult().GetDetails()->GetState());
}

TEST_P(VersionedLayerClientOnlineTest, CancelAllBatchTest) {
  auto versionedClient = createVersionedLayerClient();
  auto responseFuture =
      versionedClient
          ->StartBatch(StartBatchRequest().WithLayers(
              {CustomParameters::getArgument(kVersionedLayer)}))
          .GetFuture();

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  versionedClient->CancelAll();

  auto response = responseFuture.get();
  ASSERT_FALSE(response.IsSuccessful());
}

TEST_P(VersionedLayerClientOnlineTest, PublishToBatchTest) {
  auto versionedClient = createVersionedLayerClient();
  auto response = versionedClient
                      ->StartBatch(StartBatchRequest().WithLayers(
                          {CustomParameters::getArgument(kVersionedLayer)}))
                      .GetFuture()
                      .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto getBatchResponse =
      versionedClient->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(getBatchResponse.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            getBatchResponse.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            getBatchResponse.GetResult().GetDetails()->GetState());

  auto publishToBatchResponse =
      versionedClient
          ->PublishToBatch(
              response.GetResult(),
              PublishPartitionDataRequest()
                  .WithData(
                      std::make_shared<std::vector<unsigned char>>(20, 0x30))
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithPartitionId("1111"))
          .GetFuture()
          .get();

  ASSERT_TRUE(publishToBatchResponse.IsSuccessful());
  ASSERT_EQ("1111", publishToBatchResponse.GetResult().GetTraceID());

  auto completeBatchResponse =
      versionedClient->CompleteBatch(getBatchResponse.GetResult())
          .GetFuture()
          .get();

  ASSERT_TRUE(completeBatchResponse.IsSuccessful());

  for (int i = 0; i < 100; ++i) {
    getBatchResponse =
        versionedClient->GetBatch(response.GetResult()).GetFuture().get();

    ASSERT_TRUE(getBatchResponse.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetId().value(),
              getBatchResponse.GetResult().GetId().value());
    if (getBatchResponse.GetResult().GetDetails()->GetState() != "succeeded") {
      ASSERT_EQ("submitted",
                getBatchResponse.GetResult().GetDetails()->GetState());
    } else {
      break;
    }
  }
  ASSERT_EQ("succeeded", getBatchResponse.GetResult().GetDetails()->GetState());
}

TEST_P(VersionedLayerClientOnlineTest, PublishToBatchDeleteClientTest) {
  auto versionedClient = createVersionedLayerClient();
  auto response = versionedClient
                      ->StartBatch(StartBatchRequest().WithLayers(
                          {CustomParameters::getArgument(kVersionedLayer)}))
                      .GetFuture()
                      .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto getBatchResponse =
      versionedClient->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(getBatchResponse.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            getBatchResponse.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            getBatchResponse.GetResult().GetDetails()->GetState());

  auto publishToBatchFuture =
      versionedClient
          ->PublishToBatch(
              response.GetResult(),
              PublishPartitionDataRequest()
                  .WithData(
                      std::make_shared<std::vector<unsigned char>>(20, 0x30))
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithPartitionId("1111"))
          .GetFuture();

  auto publishToBatchFuture2 =
      versionedClient
          ->PublishToBatch(
              response.GetResult(),
              PublishPartitionDataRequest()
                  .WithData(
                      std::make_shared<std::vector<unsigned char>>(20, 0x31))
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithPartitionId("1112"))
          .GetFuture();

  versionedClient = nullptr;

  auto publishToBatchResponse = publishToBatchFuture.get();
  auto publishToBatchResponse2 = publishToBatchFuture2.get();

  ASSERT_TRUE(publishToBatchResponse.IsSuccessful());
  ASSERT_EQ("1111", publishToBatchResponse.GetResult().GetTraceID());
  ASSERT_TRUE(publishToBatchResponse2.IsSuccessful());
  ASSERT_EQ("1112", publishToBatchResponse2.GetResult().GetTraceID());

  versionedClient = createVersionedLayerClient();

  auto completeBatchResponse =
      versionedClient->CompleteBatch(getBatchResponse.GetResult())
          .GetFuture()
          .get();

  ASSERT_TRUE(completeBatchResponse.IsSuccessful());

  for (int i = 0; i < 100; ++i) {
    getBatchResponse =
        versionedClient->GetBatch(response.GetResult()).GetFuture().get();

    ASSERT_TRUE(getBatchResponse.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetId().value(),
              getBatchResponse.GetResult().GetId().value());
    if (getBatchResponse.GetResult().GetDetails()->GetState() != "succeeded") {
      ASSERT_EQ("submitted",
                getBatchResponse.GetResult().GetDetails()->GetState());
    } else {
      break;
    }
  }
  ASSERT_EQ("succeeded", getBatchResponse.GetResult().GetDetails()->GetState());
}

TEST_P(VersionedLayerClientOnlineTest, PublishToBatchMultiTest) {
  auto versionedClient = createVersionedLayerClient();
  auto response = versionedClient
                      ->StartBatch(StartBatchRequest().WithLayers(
                          {CustomParameters::getArgument(kVersionedLayer)}))
                      .GetFuture()
                      .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto getBatchResponse =
      versionedClient->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(getBatchResponse.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            getBatchResponse.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            getBatchResponse.GetResult().GetDetails()->GetState());

  auto publishToBatchFuture =
      versionedClient
          ->PublishToBatch(
              response.GetResult(),
              PublishPartitionDataRequest()
                  .WithData(
                      std::make_shared<std::vector<unsigned char>>(20, 0x30))
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithPartitionId("1111"))
          .GetFuture();

  auto publishToBatchFuture2 =
      versionedClient
          ->PublishToBatch(
              response.GetResult(),
              PublishPartitionDataRequest()
                  .WithData(
                      std::make_shared<std::vector<unsigned char>>(20, 0x31))
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithPartitionId("1112"))
          .GetFuture();

  auto publishToBatchResponse = publishToBatchFuture.get();
  auto publishToBatchResponse2 = publishToBatchFuture2.get();

  ASSERT_TRUE(publishToBatchResponse.IsSuccessful());
  ASSERT_EQ("1111", publishToBatchResponse.GetResult().GetTraceID());
  ASSERT_TRUE(publishToBatchResponse2.IsSuccessful());
  ASSERT_EQ("1112", publishToBatchResponse2.GetResult().GetTraceID());

  auto completeBatchResponse =
      versionedClient->CompleteBatch(getBatchResponse.GetResult())
          .GetFuture()
          .get();

  ASSERT_TRUE(completeBatchResponse.IsSuccessful());

  for (int i = 0; i < 100; ++i) {
    getBatchResponse =
        versionedClient->GetBatch(response.GetResult()).GetFuture().get();

    ASSERT_TRUE(getBatchResponse.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetId().value(),
              getBatchResponse.GetResult().GetId().value());
    if (getBatchResponse.GetResult().GetDetails()->GetState() != "succeeded") {
      ASSERT_EQ("submitted",
                getBatchResponse.GetResult().GetDetails()->GetState());
    } else {
      break;
    }
  }
  ASSERT_EQ("succeeded", getBatchResponse.GetResult().GetDetails()->GetState());
}

TEST_P(VersionedLayerClientOnlineTest, PublishToBatchCancelTest) {
  auto versionedClient = createVersionedLayerClient();
  auto response = versionedClient
                      ->StartBatch(StartBatchRequest().WithLayers(
                          {CustomParameters::getArgument(kVersionedLayer)}))
                      .GetFuture()
                      .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto getBatchResponse =
      versionedClient->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(getBatchResponse.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            getBatchResponse.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            getBatchResponse.GetResult().GetDetails()->GetState());

  auto publishToBatchFuture =
      versionedClient
          ->PublishToBatch(
              response.GetResult(),
              PublishPartitionDataRequest()
                  .WithData(
                      std::make_shared<std::vector<unsigned char>>(20, 0x30))
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithPartitionId("1111"))
          .GetFuture();

  versionedClient->CancelAll();

  auto publishToBatchResponse = publishToBatchFuture.get();
  ASSERT_FALSE(publishToBatchResponse.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            publishToBatchResponse.GetError().GetErrorCode());

  auto cancelBatchResponse =
      versionedClient->CancelBatch(getBatchResponse.GetResult())
          .GetFuture()
          .get();
  ASSERT_TRUE(cancelBatchResponse.IsSuccessful());

  getBatchResponse =
      versionedClient->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(getBatchResponse.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            getBatchResponse.GetResult().GetId().value());
  ASSERT_EQ("cancelled", getBatchResponse.GetResult().GetDetails()->GetState());
}

TEST_P(VersionedLayerClientOnlineTest, CheckDataExistsTest) {
  auto versionedClient = createVersionedLayerClient();
  auto fut =
      versionedClient
          ->CheckDataExists(
              CheckDataExistsRequest()
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithDataHandle("5d2082c3-9738-4de7-bde0-4a52527dab37"))
          .GetFuture();
  versionedClient = nullptr;

  auto response = fut.get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_EQ(response.GetResult(), 200);
}

TEST_P(VersionedLayerClientOnlineTest, CheckDataNotExistsTest) {
  auto versionedClient = createVersionedLayerClient();
  auto fut =
      versionedClient
          ->CheckDataExists(
              CheckDataExistsRequest()
                  .WithLayerId(CustomParameters::getArgument(kVersionedLayer))
                  .WithDataHandle("5d2082c3-9738-4de7-bde0-4a52527dab34"))
          .GetFuture();
  versionedClient = nullptr;

  auto response = fut.get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_EQ(response.GetResult(), 404);
}
