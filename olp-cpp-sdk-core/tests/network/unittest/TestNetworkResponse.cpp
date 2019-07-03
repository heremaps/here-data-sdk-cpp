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

#include <gtest/gtest.h>
#include <olp/core/network/Network.h>
#include <olp/core/network/NetworkResponse.h>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace std;
using namespace olp::network;

TEST(TestNetworkResponse, construction) {
  const std::string url = "http://somesite.com/object1/subobject";
  const int priority = 4;
  const NetworkRequest request(url, 0, priority);
  GTEST_ASSERT_EQ(url, request.Url());
  GTEST_ASSERT_EQ(0u, request.ModifiedSince());
  GTEST_ASSERT_EQ(priority, request.Priority());

  const Network::RequestId id = Network::NetworkRequestIdMin;
  const bool cancelled = true;
  const int status = 42;
  const unsigned int length = 5;
  const unsigned int offset = 7;
  const int maxAge = 123;
  const time_t expires = 0;
  const std::string err("Test");
  const std::string etag("Testing");
  const std::string contentType("TestType");
  std::vector<std::pair<std::string, std::string> > statistics;
  statistics.push_back(std::pair<std::string, std::string>("Stat1", "Val1"));
  statistics.push_back(std::pair<std::string, std::string>("Stat2", "Val2"));

  NetworkResponse response(
      id, cancelled, status, err, maxAge, expires, etag, contentType, length,
      offset, std::shared_ptr<std::ostream>(), std::move(statistics));
  GTEST_ASSERT_EQ(id, response.Id());
  GTEST_ASSERT_EQ(cancelled, response.Cancelled());
  GTEST_ASSERT_EQ(request.Url(), request.Url());
  GTEST_ASSERT_EQ(request.ModifiedSince(), request.ModifiedSince());
  GTEST_ASSERT_EQ(request.Priority(), request.Priority());
  GTEST_ASSERT_EQ(status, response.Status());
  GTEST_ASSERT_EQ(maxAge, response.MaxAge());
  GTEST_ASSERT_EQ(etag, response.Etag());
  GTEST_ASSERT_EQ(contentType, response.ContentType());
  GTEST_ASSERT_EQ(err, response.Error());
  GTEST_ASSERT_EQ(length, response.PayloadSize());
  GTEST_ASSERT_EQ(offset, response.PayloadOffset());
  GTEST_ASSERT_EQ(2u, response.Statistics().size());
  GTEST_ASSERT_EQ("Stat1", response.Statistics().at(0).first);
  GTEST_ASSERT_EQ("Val1", response.Statistics().at(0).second);
  GTEST_ASSERT_EQ("Stat2", response.Statistics().at(1).first);
  GTEST_ASSERT_EQ("Val2", response.Statistics().at(1).second);
}
