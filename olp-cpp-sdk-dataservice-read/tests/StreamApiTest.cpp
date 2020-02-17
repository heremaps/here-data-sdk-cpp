/*
 * Copyright (C) 2020 HERE Europe B.V.
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
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include "generated/api/StreamApi.h"

namespace {
using namespace ::testing;
using namespace olp;
using namespace olp::client;
using namespace olp::dataservice::read;
using namespace olp::tests::common;

std::string ApiErrorToString(const ApiError& error) {
  std::ostringstream result_stream;
  result_stream << "ERROR: code: " << static_cast<int>(error.GetErrorCode())
                << ", status: " << error.GetHttpStatusCode()
                << ", message: " << error.GetMessage();
  return result_stream.str();
}

model::StreamOffsets GetStreamOffsets() {
  model::StreamOffset offset1;
  offset1.SetPartition(7);
  offset1.SetOffset(38562);
  model::StreamOffset offset2;
  offset2.SetPartition(8);
  offset2.SetOffset(27458);
  model::StreamOffsets offsets;
  offsets.SetOffsets({offset1, offset2});
  return offsets;
}

class StreamApiTest : public Test {
 protected:
  void SetUp() override {
    network_mock_ = std::make_shared<NetworkMock>();

    OlpClientSettings settings;
    settings.network_request_handler = network_mock_;
    settings.task_scheduler =
        OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
    olp_client_.SetSettings(std::move(settings));
  }

  void TearDown() override { network_mock_.reset(); }

 protected:
  OlpClient olp_client_;
  std::shared_ptr<NetworkMock> network_mock_;
};

const std::string kBaseUrl{
    "https://some.base.url/stream/v2/catalogs/"
    "hrn:here:data::olp-here-test:hereos-internal-test-v2"};
const std::string kNodeBaseUrl{
    "https://some.node.base.url/stream/v2/catalogs/"
    "hrn:here:data::olp-here-test:hereos-internal-test-v2"};
const std::string kSubscriptionId{"test-subscription-id-123"};
const std::string kConsumerId{"test-consumer-id-987"};
const std::string kLayerId{"test-layer"};
const std::string kSerialMode{"serial"};
const std::string kParallelMode{"parallel"};
const std::string kCorrelationId{"test-correlation-id"};
const olp::http::HeaderPair kCorrelationIdHeader{"X-Correlation-Id",
                                                 kCorrelationId};

constexpr auto kUrlSubscribeNoQueryParams =
    R"(https://some.base.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/test-layer/subscribe)";

constexpr auto kUrlSubscribeWithQueryParams =
    R"(https://some.base.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/test-layer/subscribe?consumerId=test-consumer-id-987&mode=serial&subscriptionId=test-subscription-id-123)";

constexpr auto kUrlConsumeDataNoQueryParams =
    R"(https://some.node.base.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/test-layer/partitions)";

constexpr auto kUrlConsumeDataWithQueryParams =
    R"(https://some.node.base.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/test-layer/partitions?mode=parallel&subscriptionId=test-subscription-id-123)";

constexpr auto kUrlCommitOffsetsNoQueryParams =
    R"(https://some.node.base.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/test-layer/offsets)";

constexpr auto kUrlCommitOffsetsWithQueryParams =
    R"(https://some.node.base.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/test-layer/offsets?mode=parallel&subscriptionId=test-subscription-id-123)";

constexpr auto kUrlSeekToOffsetNoQueryParams =
    R"(https://some.node.base.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/test-layer/seek)";

constexpr auto kUrlSeekToOffsetWithQueryParams =
    R"(https://some.node.base.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/test-layer/seek?mode=serial&subscriptionId=test-subscription-id-123)";

constexpr auto kUrlUnsubscribe =
    R"(https://some.node.base.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/test-layer/subscribe?mode=parallel&subscriptionId=test-subscription-id-123)";

constexpr auto kHttpResponseSubscribeSucceeds =
    R"jsonString({ "nodeBaseURL": "https://some.node.base.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2", "subscriptionId": "test-subscription-id-123" })jsonString";

constexpr auto kHttpResponseConsumeDataSucceeds =
    R"jsonString({ "messages": [ { "metaData": { "partition": "314010583", "checksum": "ff7494d6f17da702862e550c907c0a91", "data": "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAABGdBTUEAALGPC", "timestamp": 1517916706 }, "offset": { "partition": 7, "offset": 38562 } }, { "metaData": { "partition": "385010413", "checksum": "19a0c709c055e268207ad71f6d4947ff", "compressedDataSize": 152417, "dataSize": 250110, "dataHandle": "1b2ca68f-d4a0-4379-8120-cd025640510c", "timestamp": 1517918813 }, "offset": { "partition": 8, "offset": 27458 } } ] })jsonString";

constexpr auto kHttpResponseSubscribeFails =
    R"jsonString({ "title": "Subscription mode not supported", "status": 400, "code": "E213002", "cause": "Subscription mode 'singleton' not supported", "action": "Retry with valid subscription mode 'serial' or 'parallel'", "correlationId": "4199533b-6290-41db-8d79-edf4f4019a74" })jsonString";

constexpr auto kHttpResponseConsumeDataFails =
    R"jsonString({ "title": "Subscription not found", "status": 404, "code": "E213003", "cause": "SubscriptionId -1920183912.123e4567-e89b-12d3-a456-556642440000 not found", "action": "Subscribe again", "correlationId": "4199533b-6290-41db-8d79-edf4f4019a74" })jsonString";

constexpr auto kHttpResponseCommitOffsetsFails =
    R"jsonString({ "title": "Unable to commit offset", "status": 409, "code": "E213028", "cause": "Unable to commit offset", "action": "Commit cannot be completed. Continue with reading and committing new messages", "correlationId": "4199533b-6290-41db-8d79-edf4f4019a74" })jsonString";

constexpr auto kHttpResponseSeekToOffsetFails =
    R"jsonString({ "title": "Realm not found", "status": 400, "code": "E213017", "cause": "App / user is not associated with a realm", "action": "Update access token and retry", "correlationId": "4199533b-6290-41db-8d79-edf4f4019a74" })jsonString";

constexpr auto kHttpResponseUnsubscribeFails =
    R"jsonString({ "error": "Unauthorized", "error_description": "Token Validation Failure - invalid time in token" })jsonString";

constexpr auto kHttpRequestBodyWithConsumerProperties =
    R"jsonString({"kafkaConsumerProperties":{"field_string":"abc","field_int":"456","field_bool":"1"}})jsonString";

constexpr auto kHttpRequestBodyWithStreamOffsets =
    R"jsonString({"offsets":[{"partition":7,"offset":38562},{"partition":8,"offset":27458}]})jsonString";

TEST_F(StreamApiTest, Subscribe) {
  {
    SCOPED_TRACE("Subscribe without optional input fields succeeds");

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(kUrlSubscribeNoQueryParams), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::CREATED),
            kHttpResponseSubscribeSucceeds));

    olp_client_.SetBaseUrl(kBaseUrl);
    std::string x_correlation_id;
    CancellationContext context;
    const auto subscribe_response = StreamApi::Subscribe(
        olp_client_, kLayerId, boost::none, boost::none, boost::none,
        boost::none, context, x_correlation_id);

    EXPECT_TRUE(subscribe_response.IsSuccessful())
        << ApiErrorToString(subscribe_response.GetError());
    EXPECT_EQ(subscribe_response.GetResult().GetNodeBaseURL(), kNodeBaseUrl);
    EXPECT_EQ(subscribe_response.GetResult().GetSubscriptionId(),
              kSubscriptionId);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe with all optional input fields succeeds");

    EXPECT_CALL(*network_mock_,
                Send(AllOf(IsPostRequest(kUrlSubscribeWithQueryParams),
                           BodyEq(kHttpRequestBodyWithConsumerProperties)),
                     _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::CREATED),
            kHttpResponseSubscribeSucceeds, {kCorrelationIdHeader}));

    ConsumerProperties subscription_properties{
        ConsumerOption("field_string", "abc"),
        ConsumerOption("field_int", 456),
        ConsumerOption("field_bool", true),
    };

    olp_client_.SetBaseUrl(kBaseUrl);
    std::string x_correlation_id;
    CancellationContext context;
    const auto subscribe_response = StreamApi::Subscribe(
        olp_client_, kLayerId, kSubscriptionId, kSerialMode, kConsumerId,
        subscription_properties, context, x_correlation_id);

    EXPECT_TRUE(subscribe_response.IsSuccessful())
        << ApiErrorToString(subscribe_response.GetError());
    EXPECT_EQ(subscribe_response.GetResult().GetNodeBaseURL(), kNodeBaseUrl);
    EXPECT_EQ(subscribe_response.GetResult().GetSubscriptionId(),
              kSubscriptionId);
    EXPECT_EQ(x_correlation_id, kCorrelationId);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe fails");

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(kUrlSubscribeNoQueryParams), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::FORBIDDEN),
            kHttpResponseSubscribeFails, {kCorrelationIdHeader}));

    olp_client_.SetBaseUrl(kBaseUrl);
    std::string x_correlation_id;
    CancellationContext context;
    const auto subscribe_response = StreamApi::Subscribe(
        olp_client_, kLayerId, boost::none, boost::none, boost::none,
        boost::none, context, x_correlation_id);

    EXPECT_FALSE(subscribe_response.IsSuccessful());
    EXPECT_EQ(subscribe_response.GetError().GetHttpStatusCode(),
              http::HttpStatusCode::FORBIDDEN);
    EXPECT_EQ(subscribe_response.GetError().GetMessage(),
              kHttpResponseSubscribeFails);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(StreamApiTest, ConsumeData) {
  {
    SCOPED_TRACE("ConsumeData without optional input fields succeeds");

    EXPECT_CALL(*network_mock_,
                Send(AllOf(IsGetRequest(kUrlConsumeDataNoQueryParams),
                           HeadersContain(kCorrelationIdHeader)),
                     _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            kHttpResponseConsumeDataSucceeds, {kCorrelationIdHeader}));

    olp_client_.SetBaseUrl(kNodeBaseUrl);
    std::string x_correlation_id = kCorrelationId;
    CancellationContext context;
    const auto consume_data_response =
        StreamApi::ConsumeData(olp_client_, kLayerId, boost::none, boost::none,
                               context, x_correlation_id);

    EXPECT_TRUE(consume_data_response.IsSuccessful())
        << ApiErrorToString(consume_data_response.GetError());
    EXPECT_EQ(consume_data_response.GetResult().GetMessages().size(), 2);
    EXPECT_EQ(x_correlation_id, kCorrelationId);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("ConsumeData with all optional input fields succeeds");

    EXPECT_CALL(*network_mock_,
                Send(AllOf(IsGetRequest(kUrlConsumeDataWithQueryParams),
                           HeadersContain(kCorrelationIdHeader)),
                     _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            kHttpResponseConsumeDataSucceeds, {kCorrelationIdHeader}));

    olp_client_.SetBaseUrl(kNodeBaseUrl);
    std::string x_correlation_id = kCorrelationId;
    CancellationContext context;
    const auto consume_data_response =
        StreamApi::ConsumeData(olp_client_, kLayerId, kSubscriptionId,
                               kParallelMode, context, x_correlation_id);

    EXPECT_TRUE(consume_data_response.IsSuccessful())
        << ApiErrorToString(consume_data_response.GetError());
    EXPECT_EQ(consume_data_response.GetResult().GetMessages().size(), 2);
    EXPECT_EQ(x_correlation_id, kCorrelationId);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("ConsumeData fails");

    EXPECT_CALL(*network_mock_,
                Send(AllOf(IsGetRequest(kUrlConsumeDataNoQueryParams),
                           HeadersContain(kCorrelationIdHeader)),
                     _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::NOT_FOUND),
            kHttpResponseConsumeDataFails));

    olp_client_.SetBaseUrl(kNodeBaseUrl);
    std::string x_correlation_id = kCorrelationId;
    CancellationContext context;
    const auto consume_data_response =
        StreamApi::ConsumeData(olp_client_, kLayerId, boost::none, boost::none,
                               context, x_correlation_id);

    EXPECT_FALSE(consume_data_response.IsSuccessful());
    EXPECT_EQ(consume_data_response.GetError().GetHttpStatusCode(),
              http::HttpStatusCode::NOT_FOUND);
    EXPECT_EQ(consume_data_response.GetError().GetMessage(),
              kHttpResponseConsumeDataFails);
    EXPECT_EQ(consume_data_response.GetResult().GetMessages().size(), 0);
    EXPECT_EQ(x_correlation_id, kCorrelationId);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(StreamApiTest, CommitOffsets) {
  const auto stream_offsets = GetStreamOffsets();

  {
    SCOPED_TRACE("CommitOffsets without optional input fields succeeds");

    EXPECT_CALL(*network_mock_,
                Send(AllOf(IsPutRequest(kUrlCommitOffsetsNoQueryParams),
                           HeadersContain(kCorrelationIdHeader),
                           BodyEq(kHttpRequestBodyWithStreamOffsets)),
                     _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK), "",
            {kCorrelationIdHeader}));

    olp_client_.SetBaseUrl(kNodeBaseUrl);
    std::string x_correlation_id = kCorrelationId;
    CancellationContext context;
    const auto commit_offsets_response = StreamApi::CommitOffsets(
        olp_client_, kLayerId, stream_offsets, boost::none, boost::none,
        context, x_correlation_id);

    EXPECT_TRUE(commit_offsets_response.IsSuccessful())
        << ApiErrorToString(commit_offsets_response.GetError());
    EXPECT_EQ(commit_offsets_response.GetResult(), http::HttpStatusCode::OK);
    EXPECT_EQ(x_correlation_id, kCorrelationId);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("CommitOffsets with all optional input fields succeeds");

    EXPECT_CALL(*network_mock_,
                Send(AllOf(IsPutRequest(kUrlCommitOffsetsWithQueryParams),
                           HeadersContain(kCorrelationIdHeader),
                           BodyEq(kHttpRequestBodyWithStreamOffsets)),
                     _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK), "",
            {kCorrelationIdHeader}));

    olp_client_.SetBaseUrl(kNodeBaseUrl);
    std::string x_correlation_id = kCorrelationId;
    CancellationContext context;
    const auto commit_offsets_response = StreamApi::CommitOffsets(
        olp_client_, kLayerId, stream_offsets, kSubscriptionId, kParallelMode,
        context, x_correlation_id);

    EXPECT_TRUE(commit_offsets_response.IsSuccessful())
        << ApiErrorToString(commit_offsets_response.GetError());
    EXPECT_EQ(commit_offsets_response.GetResult(), http::HttpStatusCode::OK);
    EXPECT_EQ(x_correlation_id, kCorrelationId);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("CommitOffsets fails");

    EXPECT_CALL(*network_mock_,
                Send(AllOf(IsPutRequest(kUrlCommitOffsetsNoQueryParams),
                           HeadersContain(kCorrelationIdHeader),
                           BodyEq(kHttpRequestBodyWithStreamOffsets)),
                     _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::CONFLICT),
            kHttpResponseCommitOffsetsFails, {kCorrelationIdHeader}));

    olp_client_.SetBaseUrl(kNodeBaseUrl);
    std::string x_correlation_id = kCorrelationId;
    CancellationContext context;
    const auto commit_offsets_response = StreamApi::CommitOffsets(
        olp_client_, kLayerId, stream_offsets, boost::none, boost::none,
        context, x_correlation_id);

    EXPECT_FALSE(commit_offsets_response.IsSuccessful());
    EXPECT_EQ(commit_offsets_response.GetError().GetHttpStatusCode(),
              http::HttpStatusCode::CONFLICT);
    EXPECT_EQ(commit_offsets_response.GetError().GetMessage(),
              kHttpResponseCommitOffsetsFails);
    EXPECT_EQ(x_correlation_id, kCorrelationId);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(StreamApiTest, SeekToOffset) {
  const auto stream_offsets = GetStreamOffsets();

  {
    SCOPED_TRACE("SeekToOffset without optional input fields succeeds");

    EXPECT_CALL(*network_mock_,
                Send(AllOf(IsPutRequest(kUrlSeekToOffsetNoQueryParams),
                           HeadersContain(kCorrelationIdHeader),
                           BodyEq(kHttpRequestBodyWithStreamOffsets)),
                     _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK), "",
            {kCorrelationIdHeader}));

    olp_client_.SetBaseUrl(kNodeBaseUrl);
    std::string x_correlation_id = kCorrelationId;
    CancellationContext context;
    const auto seek_to_offset_response = StreamApi::SeekToOffset(
        olp_client_, kLayerId, stream_offsets, boost::none, boost::none,
        context, x_correlation_id);

    EXPECT_TRUE(seek_to_offset_response.IsSuccessful())
        << ApiErrorToString(seek_to_offset_response.GetError());
    EXPECT_EQ(seek_to_offset_response.GetResult(), http::HttpStatusCode::OK);
    EXPECT_EQ(x_correlation_id, kCorrelationId);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("SeekToOffset with all optional input fields succeeds");

    EXPECT_CALL(*network_mock_,
                Send(AllOf(IsPutRequest(kUrlSeekToOffsetWithQueryParams),
                           HeadersContain(kCorrelationIdHeader),
                           BodyEq(kHttpRequestBodyWithStreamOffsets)),
                     _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK), "",
            {kCorrelationIdHeader}));

    olp_client_.SetBaseUrl(kNodeBaseUrl);
    std::string x_correlation_id = kCorrelationId;
    CancellationContext context;
    const auto commit_offsets_response = StreamApi::SeekToOffset(
        olp_client_, kLayerId, stream_offsets, kSubscriptionId, kSerialMode,
        context, x_correlation_id);

    EXPECT_TRUE(commit_offsets_response.IsSuccessful())
        << ApiErrorToString(commit_offsets_response.GetError());
    EXPECT_EQ(commit_offsets_response.GetResult(), http::HttpStatusCode::OK);
    EXPECT_EQ(x_correlation_id, kCorrelationId);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("SeekToOffset fails");

    EXPECT_CALL(*network_mock_,
                Send(AllOf(IsPutRequest(kUrlSeekToOffsetNoQueryParams),
                           HeadersContain(kCorrelationIdHeader),
                           BodyEq(kHttpRequestBodyWithStreamOffsets)),
                     _, _, _, _))
        .WillOnce(ReturnHttpResponse(http::NetworkResponse().WithStatus(
                                         http::HttpStatusCode::BAD_REQUEST),
                                     kHttpResponseSeekToOffsetFails,
                                     {kCorrelationIdHeader}));

    olp_client_.SetBaseUrl(kNodeBaseUrl);
    std::string x_correlation_id = kCorrelationId;
    CancellationContext context;
    const auto commit_offsets_response = StreamApi::SeekToOffset(
        olp_client_, kLayerId, stream_offsets, boost::none, boost::none,
        context, x_correlation_id);

    EXPECT_FALSE(commit_offsets_response.IsSuccessful());
    EXPECT_EQ(commit_offsets_response.GetError().GetHttpStatusCode(),
              http::HttpStatusCode::BAD_REQUEST);
    EXPECT_EQ(commit_offsets_response.GetError().GetMessage(),
              kHttpResponseSeekToOffsetFails);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(StreamApiTest, DeleteSubscription) {
  {
    SCOPED_TRACE("DeleteSubscription succeeds");

    EXPECT_CALL(*network_mock_,
                Send(AllOf(IsDeleteRequest(kUrlUnsubscribe),
                           HeadersContain(kCorrelationIdHeader)),
                     _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            http::NetworkResponse().WithStatus(http::HttpStatusCode::OK), ""));

    olp_client_.SetBaseUrl(kNodeBaseUrl);
    CancellationContext context;
    const auto unsubscribe_response =
        StreamApi::DeleteSubscription(olp_client_, kLayerId, kSubscriptionId,
                                      kParallelMode, kCorrelationId, context);

    EXPECT_TRUE(unsubscribe_response.IsSuccessful())
        << ApiErrorToString(unsubscribe_response.GetError());
    EXPECT_EQ(unsubscribe_response.GetResult(), http::HttpStatusCode::OK);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("DeleteSubscription fails");

    EXPECT_CALL(*network_mock_,
                Send(AllOf(IsDeleteRequest(kUrlUnsubscribe),
                           HeadersContain(kCorrelationIdHeader)),
                     _, _, _, _))
        .WillOnce(ReturnHttpResponse(http::NetworkResponse().WithStatus(
                                         http::HttpStatusCode::UNAUTHORIZED),
                                     kHttpResponseUnsubscribeFails));

    olp_client_.SetBaseUrl(kNodeBaseUrl);
    CancellationContext context;
    const auto unsubscribe_response =
        StreamApi::DeleteSubscription(olp_client_, kLayerId, kSubscriptionId,
                                      kParallelMode, kCorrelationId, context);

    EXPECT_FALSE(unsubscribe_response.IsSuccessful());
    EXPECT_EQ(unsubscribe_response.GetError().GetHttpStatusCode(),
              http::HttpStatusCode::UNAUTHORIZED);
    EXPECT_EQ(unsubscribe_response.GetError().GetMessage(),
              kHttpResponseUnsubscribeFails);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}
}  // namespace
