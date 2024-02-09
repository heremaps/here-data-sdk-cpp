/*
 * Copyright (C) 2020-2024 HERE Europe B.V.
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

#include "olp/core/http/HttpStatusCode.h"

#include "http/DefaultNetwork.h"

namespace {

using testing::_;
using testing::AllOf;
using testing::DoAll;
using testing::Mock;
using testing::Return;
using testing::SaveArg;
namespace http = olp::http;

const char* kTestUrl = "test_url";

http::Network::Statistics Statistics(uint64_t downloaded, uint64_t uploaded,
                                     uint32_t total_requests, uint32_t failed) {
  http::Network::Statistics s;
  s.bytes_downloaded = downloaded;
  s.bytes_uploaded = uploaded;
  s.total_requests = total_requests;
  s.total_failed = failed;
  return s;
}

bool CompareStatistics(const http::Network::Statistics& l,
                       const http::Network::Statistics& r) {
  return l.bytes_downloaded == r.bytes_downloaded &&
         l.bytes_uploaded == r.bytes_uploaded &&
         l.total_failed == r.total_failed &&
         l.total_requests == r.total_requests;
}

TEST(DefaultNetworkTest, Send) {
  auto network_mock = std::make_shared<NetworkMock>();
  auto default_network_adapter =
      std::make_shared<http::DefaultNetwork>(network_mock);

  {
    SCOPED_TRACE("Direct Send call");

    auto request = http::NetworkRequest(kTestUrl).WithVerb(
        http::NetworkRequest::HttpVerb::GET);

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kTestUrl), _, _, _, _))
        .WillOnce(Return(http::SendOutcome(1)));

    default_network_adapter->Send(request, nullptr, nullptr);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("Default headers only");

    auto request = http::NetworkRequest(kTestUrl).WithVerb(
        http::NetworkRequest::HttpVerb::GET);

    http::Header default_header("default-header", "default-value");
    http::Headers headers = {default_header};

    auto request_matcher =
        AllOf(IsGetRequest(kTestUrl), HeadersContain(default_header));

    EXPECT_CALL(*network_mock, Send(request_matcher, _, _, _, _))
        .WillOnce(Return(http::SendOutcome(1)));

    default_network_adapter->SetDefaultHeaders(headers);

    default_network_adapter->Send(request, nullptr, nullptr);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("Default headers appended");

    http::Header request_header("request-header", "request-value");

    auto request = http::NetworkRequest(kTestUrl)
                       .WithVerb(http::NetworkRequest::HttpVerb::GET)
                       .WithHeader(request_header.first, request_header.second);

    http::Header default_header("default-header", "default-value");
    http::Header default_user_agent("user-agent", "default_user_agent");
    http::Headers headers = {default_header, default_user_agent};

    http::Header expected_user_agent(http::kUserAgentHeader,
                                     "default_user_agent");

    auto request_matcher = AllOf(
        IsGetRequest(kTestUrl), HeadersContain(request_header),
        HeadersContain(default_header), HeadersContain(expected_user_agent));

    EXPECT_CALL(*network_mock, Send(request_matcher, _, _, _, _))
        .WillOnce(Return(http::SendOutcome(1)));

    default_network_adapter->SetDefaultHeaders(headers);

    default_network_adapter->Send(request, nullptr, nullptr);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }
  {
    SCOPED_TRACE("User agents concatenated");

    http::Header request_header("request-header", "request-value");
    http::Header request_user_agent("user-agent", "requested_user_agent");

    auto request =
        http::NetworkRequest(kTestUrl)
            .WithVerb(http::NetworkRequest::HttpVerb::GET)
            .WithHeader(request_header.first, request_header.second)
            .WithHeader(request_user_agent.first, request_user_agent.second);

    http::Header default_header("default-header", "default-value");
    http::Header default_user_agent("user-agent", "default_user_agent");
    http::Headers headers = {default_header, default_user_agent};

    http::Header expected_user_agent("user-agent",
                                     "requested_user_agent default_user_agent");

    auto request_matcher = AllOf(
        IsGetRequest(kTestUrl), HeadersContain(request_header),
        HeadersContain(default_header), HeadersContain(expected_user_agent));

    EXPECT_CALL(*network_mock, Send(request_matcher, _, _, _, _))
        .WillOnce(Return(http::SendOutcome(1)));

    default_network_adapter->SetDefaultHeaders(headers);

    default_network_adapter->Send(request, nullptr, nullptr);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }
}

TEST(DefaultNetworkTest, DefaultBucket) {
  SCOPED_TRACE("Default bucket used");

  auto network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<http::Network> default_network_adapter =
      std::make_shared<http::DefaultNetwork>(network_mock);

  auto request = http::NetworkRequest(kTestUrl).WithVerb(
      http::NetworkRequest::HttpVerb::GET);

  http::Network::Callback network_callback;

  EXPECT_CALL(*network_mock, Send(IsGetRequest(kTestUrl), _, _, _, _))
      .WillOnce(
          DoAll(SaveArg<2>(&network_callback), Return(http::SendOutcome(1))));

  http::NetworkResponse network_response;
  auto callback = [&network_response](http::NetworkResponse response) {
    network_response = std::move(response);
  };

  auto outcome = default_network_adapter->Send(request, nullptr, callback);

  EXPECT_EQ(outcome.GetRequestId(), 1ull);
  ASSERT_TRUE(network_callback);

  network_callback(http::NetworkResponse()
                       .WithBytesDownloaded(100ull)
                       .WithBytesUploaded(50ull)
                       .WithStatus(olp::http::HttpStatusCode::OK));

  EXPECT_EQ(network_response.GetBytesDownloaded(), 100ull);
  EXPECT_EQ(network_response.GetBytesUploaded(), 50ull);

  auto stats = default_network_adapter->GetStatistics();
  EXPECT_TRUE(CompareStatistics(stats, Statistics(100ull, 50ull, 1u, 0u)));
}

TEST(DefaultNetworkTest, BucketSelection) {
  SCOPED_TRACE("Active bucket used");

  auto network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<http::Network> default_network_adapter =
      std::make_shared<http::DefaultNetwork>(network_mock);

  auto request = http::NetworkRequest(kTestUrl).WithVerb(
      http::NetworkRequest::HttpVerb::GET);

  http::Network::Callback network_callback;

  EXPECT_CALL(*network_mock, Send(IsGetRequest(kTestUrl), _, _, _, _))
      .WillOnce(
          DoAll(SaveArg<2>(&network_callback), Return(http::SendOutcome(1))));

  http::NetworkResponse network_response;
  auto callback = [&network_response](http::NetworkResponse response) {
    network_response = std::move(response);
  };

  default_network_adapter->SetCurrentBucket(1);

  auto outcome = default_network_adapter->Send(request, nullptr, callback);

  default_network_adapter->SetCurrentBucket(2);

  EXPECT_EQ(outcome.GetRequestId(), 1ull);
  ASSERT_TRUE(network_callback);

  network_callback(http::NetworkResponse()
                       .WithBytesDownloaded(100ull)
                       .WithBytesUploaded(50ull)
                       .WithStatus(olp::http::HttpStatusCode::OK));

  EXPECT_EQ(network_response.GetBytesDownloaded(), 100ull);
  EXPECT_EQ(network_response.GetBytesUploaded(), 50ull);

  auto stats_1 = default_network_adapter->GetStatistics(1);
  EXPECT_TRUE(CompareStatistics(stats_1, Statistics(100ull, 50ull, 1u, 0u)));

  auto stats_2 = default_network_adapter->GetStatistics(2);
  EXPECT_TRUE(CompareStatistics(stats_2, http::Network::Statistics{}));
}

TEST(DefaultNetworkTest, FailedPrecondition) {
  SCOPED_TRACE("Failed request precondition do not affect statistics");

  auto network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<http::Network> default_network_adapter =
      std::make_shared<http::DefaultNetwork>(network_mock);

  auto request = http::NetworkRequest(kTestUrl).WithVerb(
      http::NetworkRequest::HttpVerb::GET);

  EXPECT_CALL(*network_mock, Send(IsGetRequest(kTestUrl), _, _, _, _))
      .WillOnce(
          Return(http::SendOutcome(olp::http::ErrorCode::INVALID_URL_ERROR)));

  auto outcome = default_network_adapter->Send(request, nullptr, nullptr);
  EXPECT_EQ(outcome.GetErrorCode(), olp::http::ErrorCode::INVALID_URL_ERROR);

  auto stats = default_network_adapter->GetStatistics();
  EXPECT_TRUE(CompareStatistics(stats, http::Network::Statistics{}));
}

TEST(DefaultNetworkTest, FailedResponse) {
  SCOPED_TRACE("Failed response increments total_failed");

  auto network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<http::Network> default_network_adapter =
      std::make_shared<http::DefaultNetwork>(network_mock);

  auto request = http::NetworkRequest(kTestUrl).WithVerb(
      http::NetworkRequest::HttpVerb::GET);

  http::Network::Callback network_callback;

  EXPECT_CALL(*network_mock, Send(IsGetRequest(kTestUrl), _, _, _, _))
      .WillOnce(
          DoAll(SaveArg<2>(&network_callback), Return(http::SendOutcome(1))));

  http::NetworkResponse network_response;
  auto callback = [&network_response](http::NetworkResponse response) {
    network_response = std::move(response);
  };

  auto outcome = default_network_adapter->Send(request, nullptr, callback);

  EXPECT_EQ(outcome.GetRequestId(), 1ull);
  ASSERT_TRUE(network_callback);

  network_callback(
      http::NetworkResponse()
          .WithBytesDownloaded(150ull)
          .WithBytesUploaded(250ull)
          .WithStatus(olp::http::HttpStatusCode::SERVICE_UNAVAILABLE));

  EXPECT_EQ(network_response.GetBytesDownloaded(), 150ull);
  EXPECT_EQ(network_response.GetBytesUploaded(), 250ull);

  auto stats = default_network_adapter->GetStatistics();
  EXPECT_TRUE(CompareStatistics(stats, Statistics(150ull, 250ull, 1u, 1u)));
}

}  // namespace
