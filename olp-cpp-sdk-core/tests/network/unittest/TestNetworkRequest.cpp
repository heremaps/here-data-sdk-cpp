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
#include <olp/core/network/NetworkRequest.h>
#include <olp/core/network/NetworkResponse.h>
#include <future>
#include <string>

using namespace olp::network;

TEST(TestNetworkRequest, default_construction) {
  NetworkRequest request;
  GTEST_ASSERT_EQ(std::string(), request.Url());
  GTEST_ASSERT_EQ(0u, request.ModifiedSince());
  GTEST_ASSERT_EQ(NetworkRequest::PriorityDefault, request.Priority());
}

TEST(TestNetworkRequest, construction) {
  const std::string url = "http://somesite.com/object1/subobject";
  const std::uint64_t modifiedSince = 123456;
  const int priority = 3;
  NetworkRequest request(url, modifiedSince, priority);
  GTEST_ASSERT_EQ(url, request.Url());
  GTEST_ASSERT_EQ(modifiedSince, request.ModifiedSince());
  GTEST_ASSERT_EQ(priority, request.Priority());

  NetworkRequest defaultPriorityRequest(url);
  GTEST_ASSERT_EQ(url, defaultPriorityRequest.Url());
  GTEST_ASSERT_EQ(0u, defaultPriorityRequest.ModifiedSince());
  GTEST_ASSERT_EQ(NetworkRequest::PriorityDefault,
                  defaultPriorityRequest.Priority());
}

TEST(TestNetworkRequest, construction_priority_min) {
  const std::string url = "http://somesite.com/object1/subobject";
  const int priority = -2;
  NetworkRequest request(url, 0, priority);
  GTEST_ASSERT_EQ(url, request.Url());
  GTEST_ASSERT_EQ(0u, request.ModifiedSince());
  GTEST_ASSERT_EQ(NetworkRequest::PriorityMin, request.Priority());
}

TEST(TestNetworkRequest, construction_priority_max) {
  const std::string url = "http://somesite.com/object1/subobject";
  const int priority = 200;
  NetworkRequest request(url, 0, priority);
  GTEST_ASSERT_EQ(url, request.Url());
  GTEST_ASSERT_EQ(0u, request.ModifiedSince());
  GTEST_ASSERT_EQ(NetworkRequest::PriorityMax, request.Priority());
}
