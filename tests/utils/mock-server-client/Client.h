/*
 * Copyright (C) 2020-2025 HERE Europe B.V.
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

#include <memory>
#include <string>
#include <vector>

#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/http/HttpStatusCode.h>
#include "Expectation.h"
#include "Status.h"

namespace mockserver {

const auto kBaseUrl = "https://localhost:1080";
const auto kExpectationPath = "/mockserver/expectation";
const auto kStatusPath = "/mockserver/status";
const auto kResetPath = "/mockserver/reset";
const auto kClearPath = "/mockserver/clear";
const auto kVerifySequence = "/mockserver/verifySequence";

const auto kTimeout = std::chrono::seconds{10};

class Client {
 public:
  explicit Client(olp::client::OlpClientSettings settings);

  void MockResponse(
      const std::string& method_matcher, const std::string& path_matcher,
      const std::string& response_body,
      int https_status = olp::http::HttpStatusCode::OK, bool unlimited = false,
      boost::optional<int32_t> delay_ms = boost::none,
      boost::optional<std::vector<Expectation::QueryStringParameter>>
          query_params = boost::none);

  void MockBinaryResponse(const std::string& method_matcher,
                          const std::string& path_matcher,
                          const std::string& response_body);

  Status::Ports Ports() const;

  void Reset();

  bool RemoveMockResponse(const std::string& method_matcher,
                          const std::string& path_matcher);

  bool VerifySequence(const std::vector<std::string>& pathes) const;

 private:
  void CreateExpectation(const Expectation& expectation);

 private:
  std::shared_ptr<olp::client::OlpClient> http_client_;
};

inline Client::Client(olp::client::OlpClientSettings settings) {
  http_client_ = olp::client::OlpClientFactory::Create(settings);
  http_client_->SetBaseUrl(kBaseUrl);
}

inline void Client::MockResponse(
    const std::string& method_matcher, const std::string& path_matcher,
    const std::string& response_body, int https_status, bool unlimited,
    boost::optional<int32_t> delay_ms,
    boost::optional<std::vector<Expectation::QueryStringParameter>>
        query_params) {
  auto expectation = Expectation{};
  expectation.request.path = path_matcher;
  expectation.request.method = method_matcher;
  expectation.request.query_string_parameters = std::move(query_params);

  boost::optional<Expectation::ResponseAction> action =
      Expectation::ResponseAction{};
  action->body = response_body;
  action->status_code = static_cast<uint16_t>(https_status);
  if (delay_ms) {
    action->delay = {{delay_ms.get(), "MILLISECONDS"}};
  }
  expectation.action = action;

  boost::optional<Expectation::ResponseTimes> times =
      Expectation::ResponseTimes{};
  times->remaining_times = 1;
  times->unlimited = unlimited;
  expectation.times = times;

  CreateExpectation(expectation);
}

inline void Client::MockBinaryResponse(const std::string& method_matcher,
                                       const std::string& path_matcher,
                                       const std::string& response_body) {
  auto expectation = Expectation{};
  expectation.request.path = path_matcher;
  expectation.request.method = method_matcher;

  auto binary_response = Expectation::BinaryResponse{};
  binary_response.base64_string = response_body;

  boost::optional<Expectation::ResponseAction> action =
      Expectation::ResponseAction{};
  action->body = binary_response;
  expectation.action = action;

  boost::optional<Expectation::ResponseTimes> times =
      Expectation::ResponseTimes{};
  times->remaining_times = 1;
  times->unlimited = false;
  expectation.times = times;

  CreateExpectation(expectation);
}

inline Status::Ports Client::Ports() const {
  auto response = http_client_->CallApi(kStatusPath, "PUT", {}, {}, {}, nullptr,
                                        "", olp::client::CancellationContext{});

  if (response.GetStatus() != olp::http::HttpStatusCode::OK) {
    return {};
  }

  const auto status = parse<Status>(response.GetRawResponse());
  return status.ports;
}

inline void Client::Reset() {
  auto response = http_client_->CallApi(kResetPath, "PUT", {}, {}, {}, nullptr,
                                        "", olp::client::CancellationContext{});
}

inline bool Client::RemoveMockResponse(const std::string& method_matcher,
                                       const std::string& path_matcher) {
  boost::json::object object;
  object.emplace("method", method_matcher);
  object.emplace("path", path_matcher);

  const auto data = boost::json::serialize(object);

  const auto request_body =
      std::make_shared<std::vector<unsigned char>>(data.begin(), data.end());

  auto response =
      http_client_->CallApi(kClearPath, "PUT", {}, {}, {}, request_body, "",
                            olp::client::CancellationContext{});
  if (response.GetStatus() != olp::http::HttpStatusCode::OK) {
    return false;
  }

  return true;
}

inline void Client::CreateExpectation(const Expectation& expectation) {
  const auto data = serialize(expectation);
  const std::shared_ptr<std::vector<unsigned char>> request_body =
      std::make_shared<std::vector<unsigned char>>(data.begin(), data.end());

  auto response =
      http_client_->CallApi(kExpectationPath, "PUT", {}, {}, {}, request_body,
                            "", olp::client::CancellationContext{});
}

inline bool Client::VerifySequence(
    const std::vector<std::string>& pathes) const {
  boost::json::array array;
  for (const auto& str : pathes) {
    boost::json::object array_value;
    array_value.emplace("path", str);
    array.emplace_back(std::move(array_value));
  }
  boost::json::object object;
  object.emplace("httpRequests", std::move(array));

  const auto data = boost::json::serialize(object);

  const auto request_body =
      std::make_shared<std::vector<unsigned char>>(data.begin(), data.end());

  auto response =
      http_client_->CallApi(kVerifySequence, "PUT", {}, {}, {}, request_body,
                            "", olp::client::CancellationContext{});
  if (response.GetStatus() != olp::http::HttpStatusCode::ACCEPTED) {
    return false;
  }
  return true;
}

}  // namespace mockserver
