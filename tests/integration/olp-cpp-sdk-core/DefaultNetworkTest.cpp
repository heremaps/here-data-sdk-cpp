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

#include "http/DefaultNetwork.h"

namespace {

using namespace testing;
using namespace olp::http;
using namespace olp::tests::common;

const char* kTestUrl = "test_url";

TEST(DefaultNetworkTest, Send) {
  auto network_mock = std::make_shared<NetworkMock>();
  auto default_network_adapter = std::make_shared<DefaultNetwork>(network_mock);

  {
    SCOPED_TRACE("Direct Send call");

    auto request =
        NetworkRequest(kTestUrl).WithVerb(NetworkRequest::HttpVerb::GET);

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kTestUrl), _, _, _, _))
        .WillOnce(Return(SendOutcome(1)));

    default_network_adapter->Send(request, nullptr, nullptr);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("Default headers only");

    auto request =
        NetworkRequest(kTestUrl).WithVerb(NetworkRequest::HttpVerb::GET);

    Header default_header("default-header", "default-value");
    Headers headers = {default_header};

    auto request_matcher =
        AllOf(IsGetRequest(kTestUrl), HeadersContain(default_header));

    EXPECT_CALL(*network_mock, Send(request_matcher, _, _, _, _))
        .WillOnce(Return(SendOutcome(1)));

    default_network_adapter->SetDefaultHeaders(headers);

    default_network_adapter->Send(request, nullptr, nullptr);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("Default headers appended");

    Header request_header("request-header", "request-value");

    auto request = NetworkRequest(kTestUrl)
                       .WithVerb(NetworkRequest::HttpVerb::GET)
                       .WithHeader(request_header.first, request_header.second);

    Header default_header("default-header", "default-value");
    Header default_user_agent("user-agent", "default_user_agent");
    Headers headers = {default_header, default_user_agent};

    Header expected_user_agent(kUserAgentHeader, "default_user_agent");

    auto request_matcher = AllOf(
        IsGetRequest(kTestUrl), HeadersContain(request_header),
        HeadersContain(default_header), HeadersContain(expected_user_agent));

    EXPECT_CALL(*network_mock, Send(request_matcher, _, _, _, _))
        .WillOnce(Return(SendOutcome(1)));

    default_network_adapter->SetDefaultHeaders(headers);

    default_network_adapter->Send(request, nullptr, nullptr);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }
  {
    SCOPED_TRACE("User agents concatenated");

    Header request_header("request-header", "request-value");
    Header request_user_agent("user-agent", "requested_user_agent");

    auto request =
        NetworkRequest(kTestUrl)
            .WithVerb(NetworkRequest::HttpVerb::GET)
            .WithHeader(request_header.first, request_header.second)
            .WithHeader(request_user_agent.first, request_user_agent.second);

    Header default_header("default-header", "default-value");
    Header default_user_agent("user-agent", "default_user_agent");
    Headers headers = {default_header, default_user_agent};

    Header expected_user_agent("user-agent",
                               "requested_user_agent default_user_agent");

    auto request_matcher = AllOf(
        IsGetRequest(kTestUrl), HeadersContain(request_header),
        HeadersContain(default_header), HeadersContain(expected_user_agent));

    EXPECT_CALL(*network_mock, Send(request_matcher, _, _, _, _))
        .WillOnce(Return(SendOutcome(1)));

    default_network_adapter->SetDefaultHeaders(headers);

    default_network_adapter->Send(request, nullptr, nullptr);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }
}

}  // namespace
