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
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/write/generated/model/ResponseOkSingle.h>
#include "StreamLayerClientImpl.h"

namespace {

using namespace testing;
using namespace olp;
using namespace olp::dataservice::write;
using namespace olp::tests::common;

constexpr int64_t kTwentyMib = 20971520;
const olp::client::HRN kHRN{"hrn:here:data:::catalog"};
constexpr auto kLayerName = "layer";
constexpr auto kContentType = "application/json";

struct ResponseOkWithTraceIdFrom {
  template <typename Request>
  model::ResponseOkSingle operator()(Request request) {
    model::ResponseOkSingle response;
    response.SetTraceID(*request.GetTraceId());
    return response;
  }
};

MATCHER_P(WithTraceId, trace_id, "") {
  return arg.GetTraceId() && *arg.GetTraceId() == trace_id;
}

class MockStreamLayerClientImpl : public StreamLayerClientImpl {
 public:
  using StreamLayerClientImpl::StreamLayerClientImpl;

  MockStreamLayerClientImpl(client::HRN catalog,
                            StreamLayerClientSettings client_settings,
                            client::OlpClientSettings settings)
      : StreamLayerClientImpl{catalog, client_settings, settings} {
    using namespace std::placeholders;

    ON_CALL(*this, PublishDataLessThanTwentyMibTask)
        .WillByDefault(Invoke(std::bind(
            &MockStreamLayerClientImpl::PublishDataLessThanTwentyMibTaskParent,
            this, _1, _2, _3, _4)));
  }

  MOCK_METHOD(PublishDataResponse, PublishDataLessThanTwentyMibTask,
              (client::HRN catalog, client::OlpClientSettings settings,
               model::PublishDataRequest request,
               client::CancellationContext context),
              (override));

  MOCK_METHOD(PublishDataResponse, PublishDataGreaterThanTwentyMibTask,
              (client::HRN catalog, client::OlpClientSettings settings,
               model::PublishDataRequest request,
               client::CancellationContext context),
              (override));

  MOCK_METHOD(CatalogResponse, GetCatalog,
              (client::HRN catalog, client::CancellationContext context,
               model::PublishDataRequest request,
               client::OlpClientSettings settings),
              (override));

  MOCK_METHOD(IngestDataResponse, IngestData,
              (client::HRN catalog, client::CancellationContext context,
               model::PublishDataRequest request, std::string content_type,
               client::OlpClientSettings settings),
              (override));

  PublishDataResponse PublishDataLessThanTwentyMibTaskParent(
      client::HRN catalog, client::OlpClientSettings settings,
      model::PublishDataRequest request, client::CancellationContext context) {
    return StreamLayerClientImpl::PublishDataLessThanTwentyMibTask(
        std::move(catalog), std::move(settings), std::move(request),
        std::move(context));
  }
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

TEST_F(StreamLayerClientImplTest, SmallPublishData) {
  MockStreamLayerClientImpl client{kHRN, StreamLayerClientSettings{},
                                   settings_};

  EXPECT_CALL(client, PublishDataLessThanTwentyMibTask(_, _, _, _))
      .WillOnce(Return(model::ResponseOkSingle{}));

  model::PublishDataRequest request;
  request.WithData(
      std::make_shared<std::vector<unsigned char>>(kTwentyMib - 1, 0xff));

  auto publish_data_future = client.PublishData(request).GetFuture();
  auto result = publish_data_future.get();
  EXPECT_TRUE(result.IsSuccessful()) << result.GetError().GetMessage();
}

TEST_F(StreamLayerClientImplTest, LargePublishData) {
  MockStreamLayerClientImpl client{kHRN, StreamLayerClientSettings{},
                                   settings_};

  EXPECT_CALL(client, PublishDataGreaterThanTwentyMibTask(_, _, _, _))
      .WillOnce(Return(model::ResponseOkSingle{}));

  model::PublishDataRequest request;
  request.WithData(
      std::make_shared<std::vector<unsigned char>>(kTwentyMib + 1, 0xff));

  auto publish_data_future = client.PublishData(request).GetFuture();
  auto result = publish_data_future.get();
  EXPECT_TRUE(result.IsSuccessful()) << result.GetError().GetMessage();
}

TEST_F(StreamLayerClientImplTest, IncorrectPublishData) {
  MockStreamLayerClientImpl client{kHRN, StreamLayerClientSettings{},
                                   settings_};

  ON_CALL(client, PublishDataGreaterThanTwentyMibTask(_, _, _, _))
      .WillByDefault(Return(model::ResponseOkSingle{}));

  ON_CALL(client, PublishDataLessThanTwentyMibTask(_, _, _, _))
      .WillByDefault(Return(model::ResponseOkSingle{}));

  model::PublishDataRequest request;

  auto publish_data_future = client.PublishData(request).GetFuture();
  auto result = publish_data_future.get();
  EXPECT_FALSE(result.IsSuccessful());
}

TEST_F(StreamLayerClientImplTest, QueueAndFlush) {
  const auto batch_size = 10;
  std::map<std::string, std::string> storage;
  settings_.cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache({});

  auto client = std::make_shared<MockStreamLayerClientImpl>(
      kHRN, StreamLayerClientSettings{}, settings_);

  // Forward trace ID from request to response
  ON_CALL(*client, PublishDataLessThanTwentyMibTask)
      .WillByDefault(WithArg<2>(Invoke(ResponseOkWithTraceIdFrom{})));

  EXPECT_CALL(*client, PublishDataLessThanTwentyMibTask(_, _, _, _))
      .Times(batch_size);

  for (int i = 0; i < batch_size; ++i) {
    model::PublishDataRequest request;
    request.WithTraceId(std::to_string(i));
    request.WithData(std::make_shared<std::vector<unsigned char>>(16, 0xff));
    request.WithLayerId("layer");

    auto error = client->Queue(request);
    EXPECT_EQ(boost::none, error) << *error;
  }

  EXPECT_EQ(client->QueueSize(), batch_size);

  auto flushed = std::make_shared<std::promise<void>>();

  auto response = client->Flush(model::FlushRequest()).GetFuture().get();
  // client->Flush(model::FlushRequest(),
  //               [=](StreamLayerClient::FlushResponse response) {
  // Has 10 responses
  EXPECT_EQ(response.size(), batch_size);
  std::set<std::string> trace_ids;
  // All they are successful
  for (const auto &r : response) {
    EXPECT_TRUE(r.IsSuccessful());
    trace_ids.insert(r.GetResult().GetTraceID());
  }
  // And has different trace_ids
  EXPECT_EQ(batch_size, trace_ids.size());
  //                 flushed->set_value();
  //               });

  // flushed->get_future().get();
}

TEST_F(StreamLayerClientImplTest, SmallPublishDataWithApi) {
  const auto trace_id = "123";

  std::vector<model::Layer> layers(1);
  layers[0].SetId(kLayerName);
  layers[0].SetContentType(kContentType);

  model::Catalog catalog;
  catalog.SetLayers(layers);

  auto client = std::make_shared<MockStreamLayerClientImpl>(
      kHRN, StreamLayerClientSettings{}, settings_);

  EXPECT_CALL(*client, PublishDataLessThanTwentyMibTask).Times(1);
  EXPECT_CALL(*client, GetCatalog).WillOnce(Return(catalog));
  EXPECT_CALL(*client,
              IngestData(_, _, WithTraceId(trace_id), Eq(kContentType), _))
      .WillOnce(WithArg<2>(Invoke(ResponseOkWithTraceIdFrom{})));

  model::PublishDataRequest request;
  request.WithTraceId(trace_id);
  request.WithLayerId(kLayerName);
  request.WithData(
      std::make_shared<std::vector<unsigned char>>(kTwentyMib - 1, 0xff));

  auto publish_data_future = client->PublishData(request).GetFuture();
  auto result = publish_data_future.get();

  EXPECT_TRUE(result.IsSuccessful()) << result.GetError().GetMessage();
}

}  // namespace
