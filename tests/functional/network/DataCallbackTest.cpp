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

#include <memory>

#include <gtest/gtest.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/Network.h>
#include <olp/core/http/NetworkSettings.h>

#include "NetworkTestBase.h"
#include "ReadDefaultResponses.h"

namespace {
using NetworkRequest = olp::http::NetworkRequest;
using NetworkResponse = olp::http::NetworkResponse;
using NetworkSettings = olp::http::NetworkSettings;

using DataCallbackTest = NetworkTestBase;

TEST_F(DataCallbackTest, DataChunks) {
  const std::string kUrlBase = "https://some-url.com";
  const std::string kApiBase = "/some-api";

  // generate 1mb of data
  constexpr auto data_size = 1u * 1024u * 1024u;
  auto data = mockserver::ReadDefaultResponses::GenerateData(data_size);
  auto payload = std::make_shared<std::stringstream>();
  std::vector<uint8_t> chunk_data(data_size);

  // we expect multiple data chunks are received in the test.
  int chunk_count = 0;

  mock_server_client_->MockResponse("GET", kApiBase, data);

  const auto url = kUrlBase + kApiBase;
  const auto request = NetworkRequest(url).WithSettings(settings_).WithVerb(
      olp::http::NetworkRequest::HttpVerb::GET);

  std::promise<NetworkResponse> promise;
  const auto outcome = network_->Send(
      request, payload,
      [&promise](NetworkResponse response) {
        promise.set_value(std::move(response));
      },
      nullptr,
      [&chunk_data, &chunk_count](const std::uint8_t* data,
                                  std::uint64_t offset, std::size_t length) {
        std::copy(data, data + length, &chunk_data[offset]);
        ++chunk_count;
      });
  ASSERT_TRUE(outcome.IsSuccessful());

  auto future = promise.get_future();
  const auto response = future.get();
  const auto& payload_data = payload->str();

  ASSERT_EQ(response.GetStatus(), olp::http::HttpStatusCode::OK);
  ASSERT_GT(chunk_count, 2);
  EXPECT_TRUE(std::equal(data.begin(), data.end(), chunk_data.begin()));
  EXPECT_EQ(data, payload_data);
}

}  // namespace
