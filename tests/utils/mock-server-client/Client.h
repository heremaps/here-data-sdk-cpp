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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/generated/parser/JsonParser.h>
#include "Expectation.h"
#include "Status.h"

namespace mockserver {

const auto kBaseUrl = "https://localhost:1080";
const auto kExpectationPath = "/mockserver/expectation";
const auto kStatusPath = "/mockserver/status";
const auto kResetPath = "/mockserver/reset";
const auto kTimeout = std::chrono::seconds{10};

class Client {
 public:
  explicit Client(olp::client::OlpClientSettings settings) {
    http_client_ = olp::client::OlpClientFactory::Create(settings);
    http_client_->SetBaseUrl(kBaseUrl);
  }

  void MockResponse(const std::string& method_matcher,
                    const std::string& path_matcher,
                    const std::string& response_body) {
    auto expectation = Expectation{};
    expectation.request.path = path_matcher;
    expectation.request.method = method_matcher;

    boost::optional<Expectation::ResponseAction> action =
        Expectation::ResponseAction{};
    action->body = response_body;
    expectation.action = action;

    CreateExpectation(expectation);
  }

  std::vector<int32_t> Ports() const {
    auto response =
        http_client_->CallApi(kStatusPath, "PUT", {}, {}, {}, nullptr, "", {});

    if (response.status != olp::http::HttpStatusCode::OK) {
      return {};
    }

    const auto status = olp::parser::parse<Status>(response.response);

    return status.ports;
  }

  void Reset() {
    auto response =
        http_client_->CallApi(kResetPath, "PUT", {}, {}, {}, nullptr, "", {});

    return;
  }

 private:
  void CreateExpectation(const Expectation& expectation) {
    const auto data = serialize(expectation);
    const std::shared_ptr<std::vector<unsigned char>> request_body =
        std::make_shared<std::vector<unsigned char>>(data.begin(), data.end());

    auto response = http_client_->CallApi(kExpectationPath, "PUT", {}, {}, {},
                                          request_body, "", {});

    return;
  }

 private:
  std::shared_ptr<olp::client::OlpClient> http_client_;
};
}  // namespace mockserver
