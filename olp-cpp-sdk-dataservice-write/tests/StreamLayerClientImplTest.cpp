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
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include "StreamLayerClientImpl.h"

namespace {

using namespace testing;
using namespace olp;
using namespace olp::dataservice::write;
using namespace olp::tests::common;

const olp::client::HRN kHRN{"hrn:here:data:::catalog"};
constexpr auto kLayerName = "layer";

class MockStreamLayerClientImpl : public StreamLayerClientImpl {
 public:
  using StreamLayerClientImpl::StreamLayerClientImpl;

  MOCK_METHOD(PublishSdiiResponse, IngestSdii,
              (model::PublishSdiiRequest request,
               client::CancellationContext context),
              (override));
};

class StreamLayerClientImplTest : public ::testing::Test {
 protected:
  void SetUp() override {
    cache_ = std::make_shared<CacheMock>();
    network_ = std::make_shared<NetworkMock>();

    settings_.network_request_handler = network_;
    settings_.cache = cache_;
    settings_.task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
  }

  void TearDown() override {
    settings_.network_request_handler.reset();
    settings_.cache.reset();

    network_.reset();
    cache_.reset();
  }

  std::shared_ptr<CacheMock> cache_;
  std::shared_ptr<NetworkMock> network_;
  olp::client::OlpClientSettings settings_;
};

TEST_F(StreamLayerClientImplTest, PublishSdii) {
  const auto trace_id = "123";

  model::PublishSdiiRequest request;
  request.WithSdiiMessageList(std::make_shared<std::vector<unsigned char>>());
  request.WithLayerId(kLayerName);
  request.WithTraceId(trace_id);

  auto client = std::make_shared<MockStreamLayerClientImpl>(
      kHRN, StreamLayerClientSettings{}, settings_);

  EXPECT_CALL(*client, IngestSdii).WillOnce(Return(model::ResponseOk{}));

  {
    auto good_request = request;
    auto result = client->PublishSdii(good_request).GetFuture().get();
    EXPECT_TRUE(result.IsSuccessful());
  }

  {
    auto bad_request = request;
    auto result = client->PublishSdii(bad_request.WithSdiiMessageList(nullptr))
                      .GetFuture()
                      .get();
    EXPECT_FALSE(result.IsSuccessful());
    EXPECT_EQ(result.GetError().GetErrorCode(),
              client::ErrorCode::InvalidArgument);
  }

  {
    auto bad_request = request;
    auto result =
        client->PublishSdii(bad_request.WithLayerId("")).GetFuture().get();
    EXPECT_FALSE(result.IsSuccessful());
    EXPECT_EQ(result.GetError().GetErrorCode(),
              client::ErrorCode::InvalidArgument);
  }
}
}  // namespace