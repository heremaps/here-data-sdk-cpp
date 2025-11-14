/*
 * Copyright (C) 2025 HERE Europe B.V.
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
#include <olp/core/porting/any.h>
#include <string>

TEST(AnyTest, AnyCastConstReference) {
  // Test any_cast with const reference (covers line 84 in any.h)
  const olp::porting::any const_any = std::string("test_value");

  auto result = olp::porting::any_cast<std::string>(const_any);
  EXPECT_EQ("test_value", result);
}

TEST(AnyTest, AnyCastNonConstReference) {
  // Test any_cast with non-const reference
  olp::porting::any any_obj = std::string("test_value");

  auto result = olp::porting::any_cast<std::string>(any_obj);
  EXPECT_EQ("test_value", result);
}

TEST(AnyTest, AnyCastRValueReference) {
  // Test any_cast with rvalue reference
  auto result = olp::porting::any_cast<std::string>(
      olp::porting::any(std::string("test_value")));
  EXPECT_EQ("test_value", result);
}

TEST(AnyTest, AnyCastConstPointer) {
  // Test any_cast with const pointer
  const olp::porting::any any_obj = std::string("test_value");

  const std::string* ptr = olp::porting::any_cast<std::string>(&any_obj);
  ASSERT_NE(nullptr, ptr);
  EXPECT_EQ("test_value", *ptr);
}

TEST(AnyTest, AnyCastNonConstPointer) {
  // Test any_cast with non-const pointer
  olp::porting::any any_obj = std::string("test_value");

  std::string* ptr = olp::porting::any_cast<std::string>(&any_obj);
  ASSERT_NE(nullptr, ptr);
  EXPECT_EQ("test_value", *ptr);
}

TEST(AnyTest, HasValue) {
  // Test has_value function
  olp::porting::any empty_any;
  EXPECT_FALSE(olp::porting::has_value(empty_any));

  olp::porting::any filled_any = std::string("test");
  EXPECT_TRUE(olp::porting::has_value(filled_any));
}

TEST(AnyTest, Reset) {
  // Test reset function
  olp::porting::any any_obj = std::string("test_value");
  EXPECT_TRUE(olp::porting::has_value(any_obj));

  olp::porting::reset(any_obj);
  EXPECT_FALSE(olp::porting::has_value(any_obj));
}

TEST(AnyTest, MakeAny) {
  // Test make_any with variadic arguments
  auto any_obj = olp::porting::make_any<std::string>("test");
  auto result = olp::porting::any_cast<std::string>(any_obj);
  EXPECT_EQ("test", result);
}

TEST(AnyTest, MakeAnyWithInitializerList) {
  // Test make_any with initializer list
  auto any_obj = olp::porting::make_any<std::vector<int>>({1, 2, 3, 4, 5});
  auto result = olp::porting::any_cast<std::vector<int>>(any_obj);
  EXPECT_EQ(5u, result.size());
  EXPECT_EQ(1, result[0]);
  EXPECT_EQ(5, result[4]);
}
