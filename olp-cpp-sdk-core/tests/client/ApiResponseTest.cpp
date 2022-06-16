/*
 * Copyright (C) 2022 HERE Europe B.V.
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

#include <olp/core/client/ApiResponse.h>

#include <gtest/gtest.h>

namespace {

struct PayloadT {
  int v;
};

struct PrivateClass {
  explicit PrivateClass(int v) : v_(v) {}
  PrivateClass(const PrivateClass&) = delete;
  PrivateClass(PrivateClass&&) = default;

  int v_;
};

using ResultType = int;
using ErrorType = std::string;

using olp::client::ApiResponse;

using IntResponse = ApiResponse<ResultType, ErrorType>;
using ExtendedIntResponse = ApiResponse<ResultType, ErrorType, PayloadT>;

inline bool operator==(const PayloadT& l, const PayloadT& r) {
  return l.v == r.v;
}

TEST(ApiResponseTest, Payload) {
  ExtendedIntResponse extended_response_1(1, PayloadT{2});
  ExtendedIntResponse extended_response_2 = extended_response_1;
  EXPECT_EQ(extended_response_1.GetPayload(), extended_response_2.GetPayload());
  ExtendedIntResponse extended_response_3(IntResponse(2),
                                          extended_response_2.GetPayload());
  EXPECT_EQ(extended_response_1.GetPayload(), extended_response_3.GetPayload());
}

TEST(ApiResponseTest, ResponseSlicing) {
  ExtendedIntResponse extended_response_1(1, PayloadT{2});

  IntResponse sliced_response_1(extended_response_1);
  EXPECT_EQ(sliced_response_1.GetResult(), 1);
  EXPECT_EQ(sliced_response_1.IsSuccessful(),
            extended_response_1.IsSuccessful());

  IntResponse sliced_response_2 = extended_response_1;
  EXPECT_EQ(sliced_response_2.GetResult(), 1);
  EXPECT_EQ(sliced_response_2.IsSuccessful(),
            extended_response_1.IsSuccessful());

  ExtendedIntResponse extended_response_2("error", PayloadT{2});

  IntResponse sliced_response_3(extended_response_2);
  EXPECT_EQ(sliced_response_3.GetError(), "error");
  EXPECT_EQ(sliced_response_3.IsSuccessful(),
            extended_response_2.IsSuccessful());

  IntResponse sliced_response_4 = extended_response_2;
  EXPECT_EQ(sliced_response_4.GetError(), "error");
  EXPECT_EQ(sliced_response_4.IsSuccessful(),
            extended_response_2.IsSuccessful());
}

TEST(ApiResponseTest, ResponseExtention) {
  IntResponse normal_response_1(123);

  ExtendedIntResponse extended_response_1(normal_response_1);
  EXPECT_EQ(extended_response_1.GetResult(), 123);
  EXPECT_EQ(extended_response_1.IsSuccessful(),
            normal_response_1.IsSuccessful());

  ExtendedIntResponse extended_response_2 = normal_response_1;
  EXPECT_EQ(extended_response_2.GetResult(), 123);
  EXPECT_EQ(extended_response_2.IsSuccessful(),
            normal_response_1.IsSuccessful());

  ExtendedIntResponse extended_response_3(normal_response_1, PayloadT{234});
  EXPECT_EQ(extended_response_3.GetResult(), 123);
  EXPECT_EQ(extended_response_3.GetPayload(), PayloadT{234});

  IntResponse normal_response_2("error");

  ExtendedIntResponse extended_response_4(normal_response_2);
  EXPECT_EQ(extended_response_4.GetError(), "error");
  EXPECT_EQ(extended_response_4.IsSuccessful(),
            normal_response_2.IsSuccessful());

  ExtendedIntResponse extended_response_5 = normal_response_2;
  EXPECT_EQ(extended_response_5.GetError(), "error");
  EXPECT_EQ(extended_response_5.IsSuccessful(),
            normal_response_2.IsSuccessful());

  ExtendedIntResponse extended_response_6(normal_response_2, PayloadT{234});
  EXPECT_EQ(extended_response_6.GetError(), "error");
  EXPECT_EQ(extended_response_6.GetPayload(), PayloadT{234});
}

TEST(ApiResponseTest, ResultWithoutCopyCtor) {
  using ProvateResponse = ApiResponse<PrivateClass, ErrorType, PayloadT>;
  // move constructed
  ProvateResponse response_1(PrivateClass(1));
  // move asigned
  ProvateResponse response_2 = PrivateClass(1);
  // response can be moved
  ProvateResponse response_3 = std::move(response_2);
}
}  // namespace
