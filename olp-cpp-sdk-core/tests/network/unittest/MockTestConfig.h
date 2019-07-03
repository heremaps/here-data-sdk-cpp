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

#pragma once

#include <olp/core/context/Context.h>
#include <olp/core/network/NetworkResponse.h>
#include <olp/core/porting/make_unique.h>
#include "../mock/NetworkProtocolMock.h"

#include <gtest/gtest.h>

#include <functional>
#include <future>
#include <memory>
#include <vector>

namespace olp {
namespace network {
namespace test {
struct MockNetworkRequestBuilder {
  MockNetworkRequestBuilder(NetworkProtocolMock& protocolMock)
      : m_protocolMock(protocolMock) {}

  /// functions to be used to match the concrete mock expectation
  typedef std::function<bool(const NetworkRequest&)> RequestCheckerCallback;
  MockNetworkRequestBuilder& forUrl(const std::string& url) {
    if (!url.empty())
      return forMatchedRequest([url](const NetworkRequest& request) {
        return url == request.Url();
      });
    else
      return *this;
  }

  MockNetworkRequestBuilder& forMatchedRequest(
      RequestCheckerCallback requestChecker) {
    m_requestChecker = std::move(requestChecker);
    return *this;
  }

  MockNetworkRequestBuilder& checkRepeatedly(bool check, int checkTime) {
    m_checkRepeatedly = check;
    m_checkTime = checkTime;
    return *this;
  }

  /// data setters for the response
  typedef std::function<void(std::ostream&)> DataWriterCallback;

  MockNetworkRequestBuilder& withResponseData(
      const std::vector<char>& payloadData) {
    return withResponseData(
        [payloadData](std::ostream& payloadStream) {
          payloadStream.write(payloadData.data(), payloadData.size());
        },
        payloadData.size());
  }

  MockNetworkRequestBuilder& withResponseData(
      DataWriterCallback payloadWriterCallback, size_t dataSize) {
    m_data->m_responseDataSize = dataSize;
    m_data->m_payloadWriter = std::move(payloadWriterCallback);
    return *this;
  }

  /// generic attributes of network response
  MockNetworkRequestBuilder& withReturnCode(int code) {
    m_data->m_code = code;
    return *this;
  }

  MockNetworkRequestBuilder& withErrorString(std::string errorString) {
    m_data->m_errorString = std::move(errorString);
    return *this;
  }

  MockNetworkRequestBuilder& withMaxAge(int maxAge) {
    m_data->m_maxAge = maxAge;
    return *this;
  }

  MockNetworkRequestBuilder& withExpiryTime(time_t expiry) {
    m_data->m_expiry = expiry;
    return *this;
  }

  MockNetworkRequestBuilder& withEtag(std::string etag) {
    m_data->m_etag = std::move(etag);
    return *this;
  }

  MockNetworkRequestBuilder& withContentType(std::string contentType) {
    m_data->m_contentType = std::move(contentType);
    return *this;
  }

  MockNetworkRequestBuilder& withHeaderResponse(std::string key,
                                                std::string value) {
    m_data->m_headerResponses.emplace_back(std::move(key), std::move(value));
    return *this;
  }

  /// do not use deferred finalization - invoke callback synchronously
  MockNetworkRequestBuilder& completeSynchronously() {
    m_synchronousCompletion = true;
    return *this;
  }

  /// main method: initializes the expectation according to the state
  void buildExpectation(
      NetworkProtocol::ErrorCode result = NetworkProtocol::ErrorNone) {
    ASSERT_FALSE(
        m_data->m_promise);  // should not be called again for same expectation

    if (!m_synchronousCompletion && result == NetworkProtocol::ErrorNone) {
      m_data->m_promise =
          std::make_shared<RequestCompletionPromise::element_type>();
    }

    auto data = m_data;
    auto fakeSender =
        [data, result](
            const NetworkRequest& /*request*/, int id,
            const std::shared_ptr<std::ostream>& payload,
            std::shared_ptr<NetworkConfig> /*config*/,
            Network::HeaderCallback headerCallback,
            Network::DataCallback /*dataCallback*/,
            Network::Callback callback) -> NetworkProtocol::ErrorCode {
      if (result == NetworkProtocol::ErrorNone) {
        if (payload && data->m_payloadWriter) {
          data->m_payloadWriter(*payload);
        }
        if (headerCallback) {
          for (auto& header : data->m_headerResponses)
            headerCallback(header.first, header.second);
        }
        if (data->m_promise) {
          data->m_promise->set_value([=]() {
            auto response = NetworkResponse(
                id, false, data->m_code, data->m_errorString, data->m_maxAge,
                data->m_expiry, data->m_etag, data->m_contentType,
                data->m_responseDataSize, data->m_offset, payload,
                std::move(data->m_statistics));
            callback(std::move(response));
          });
        } else {
          auto response = NetworkResponse(
              id, false, data->m_code, data->m_errorString, data->m_maxAge,
              data->m_expiry, data->m_etag, data->m_contentType,
              data->m_responseDataSize, data->m_offset, payload,
              std::move(data->m_statistics));
          callback(std::move(response));
        }
      }
      return result;
    };

    using ::testing::_;
    using ::testing::Truly;

    if (m_requestChecker) {
      if (!m_checkRepeatedly)
        EXPECT_CALL(m_protocolMock,
                    Send(Truly(m_requestChecker), _, _, _, _, _, _))
            .Times(1)
            .WillOnce(::testing::Invoke(fakeSender))
            .RetiresOnSaturation();

      else
        EXPECT_CALL(m_protocolMock,
                    Send(Truly(m_requestChecker), _, _, _, _, _, _))
            .Times(m_checkTime)
            .WillRepeatedly(::testing::Invoke(fakeSender))
            .RetiresOnSaturation();
    } else {
      if (!m_checkRepeatedly)
        EXPECT_CALL(m_protocolMock, Send(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(::testing::Invoke(fakeSender))
            .RetiresOnSaturation();
      else
        EXPECT_CALL(m_protocolMock, Send(_, _, _, _, _, _, _))
            .Times(m_checkTime)
            .WillRepeatedly(::testing::Invoke(fakeSender))
            .RetiresOnSaturation();
    }
  }

  /// for async requests, run completion
  void finalizeRequest(
      const std::chrono::seconds& waitTimeout = std::chrono::seconds(10)) {
    if (m_data->m_promise) {
      auto fut = m_data->m_promise->get_future();
      ASSERT_EQ(std::future_status::ready, fut.wait_for(waitTimeout));
      fut.get()();
      m_data->m_promise.reset();
    }
  }

 private:
  NetworkProtocolMock& m_protocolMock;

  typedef std::shared_ptr<std::promise<std::function<void()> > >
      RequestCompletionPromise;

  bool m_checkRepeatedly = false;
  int m_checkTime = 1;

  struct RequestData {
    RequestCompletionPromise m_promise;

    DataWriterCallback m_payloadWriter;

    // following ones are parameters necessary for olp::NetworkResponse
    int m_code = 200;
    std::string m_errorString;
    int m_maxAge = -1;
    time_t m_expiry = -1;
    std::string m_etag;
    std::string m_contentType;
    std::uint64_t m_responseDataSize = 0;
    std::uint64_t m_offset = 0;
    NetworkProtocol::StatisticsData m_statistics;
    std::vector<std::pair<std::string, std::string> > m_headerResponses;
  };

  std::shared_ptr<RequestData> m_data = std::make_shared<RequestData>();

  RequestCheckerCallback m_requestChecker;
  bool m_synchronousCompletion = false;
};

struct MockNetworkTestApp {
  void setUp() {
    setUp(std::make_shared<olp::network::test::NetworkProtocolMockFactory>());
  }

  void setUp(std::shared_ptr<olp::network::test::NetworkProtocolMockFactory>
                 protocolMockFactory) {
    m_protocolMock = protocolMockFactory->m_network_protocol_mock;
    olp::network::NetworkFactory::SetNetworkProtocolFactory(
        protocolMockFactory);
    EXPECT_CALL(*m_protocolMock, Initialize())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*m_protocolMock, Ready())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*m_protocolMock, Deinitialize()).Times(::testing::AnyNumber());
    EXPECT_CALL(*m_protocolMock, AmountPending())
        .WillRepeatedly(::testing::Return(0));
    m_context = std::make_unique<olp::context::Context::Scope>();
  }

  void tearDown() {
    olp::network::NetworkFactory::SetNetworkProtocolFactory(nullptr);
    m_context.reset();
    m_protocolMock.reset();
  }

  MockNetworkRequestBuilder makeExpectation() {
    return MockNetworkRequestBuilder(*m_protocolMock);
  }

  void protocolExpectNoRequests() {
    using ::testing::_;

    EXPECT_CALL(*m_protocolMock, Send(_, _, _, _, _, _, _)).Times(0);
  }

  std::unique_ptr<olp::context::Context::Scope> m_context;
  std::shared_ptr<olp::network::test::NetworkProtocolMock> m_protocolMock;
};

}  // namespace test
}  // namespace network
}  // namespace olp
