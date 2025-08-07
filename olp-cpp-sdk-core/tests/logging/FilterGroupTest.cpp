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

#include <gtest/gtest.h>
#include <sstream>

#include <olp/core/logging/FilterGroup.h>
#include <olp/core/logging/Log.h>
#include <boost/optional/optional_io.hpp>
#include "TypesToStream.h"

namespace {

using olp::logging::FilterGroup;
using olp::logging::Level;
using olp::logging::Log;

TEST(FilterGroupTest, DefaultLevel) {
  FilterGroup filter_group;
  EXPECT_EQ(olp::porting::none, filter_group.getLevel());
  filter_group.setLevel(Level::Info);
  ASSERT_NE(olp::porting::none, filter_group.getLevel());
  EXPECT_EQ(Level::Info, *filter_group.getLevel());

  filter_group.clearLevel();
  EXPECT_EQ(olp::porting::none, filter_group.getLevel());
}

TEST(FilterGroupTest, TagLevels) {
  FilterGroup filter_group;
  filter_group.setLevel(Level::Info, "test1");
  filter_group.setLevel(Level::Warning, "test2");

  ASSERT_NE(olp::porting::none, filter_group.getLevel("test1"));
  EXPECT_EQ(Level::Info, *filter_group.getLevel("test1"));
  ASSERT_NE(olp::porting::none, filter_group.getLevel("test2"));
  EXPECT_EQ(Level::Warning, *filter_group.getLevel("test2"));

  EXPECT_EQ(olp::porting::none, filter_group.getLevel("asdf"));
  filter_group.setLevel(Level::Error, "test1");

  ASSERT_NE(olp::porting::none, filter_group.getLevel("test1"));
  EXPECT_EQ(Level::Error, *filter_group.getLevel("test1"));

  filter_group.clearLevel("test1");
  EXPECT_EQ(olp::porting::none, filter_group.getLevel("test1"));
  ASSERT_NE(olp::porting::none, filter_group.getLevel("test2"));
  EXPECT_EQ(Level::Warning, *filter_group.getLevel("test2"));
}

TEST(FilterGroupTest, Clear) {
  FilterGroup filter_group;
  filter_group.setLevel(Level::Debug);
  filter_group.setLevel(Level::Info, "test1");
  filter_group.setLevel(Level::Warning, "test2");

  filter_group.clear();
  EXPECT_EQ(olp::porting::none, filter_group.getLevel());
  EXPECT_EQ(olp::porting::none, filter_group.getLevel("test1"));
  EXPECT_EQ(olp::porting::none, filter_group.getLevel("test2"));
}

TEST(FilterGroupTest, Apply) {
  Log::setLevel(Level::Debug);
  Log::setLevel(Level::Info, "test2");
  Log::setLevel(Level::Warning, "test3");
  EXPECT_TRUE(Log::isEnabled(Level::Debug, "test1"));
  EXPECT_TRUE(Log::isEnabled(Level::Info, "test2"));
  EXPECT_TRUE(Log::isEnabled(Level::Warning, "test3"));

  FilterGroup filter_group;
  filter_group.setLevel(Level::Fatal);
  filter_group.setLevel(Level::Info, "test1");
  filter_group.setLevel(Level::Warning, "test2");
  Log::applyFilterGroup(filter_group);
  EXPECT_FALSE(Log::isEnabled(Level::Debug, "test1"));
  EXPECT_FALSE(Log::isEnabled(Level::Info, "test2"));
  EXPECT_FALSE(Log::isEnabled(Level::Warning, "test3"));

  EXPECT_TRUE(Log::isEnabled(Level::Info, "test1"));
  EXPECT_TRUE(Log::isEnabled(Level::Warning, "test2"));
  EXPECT_TRUE(Log::isEnabled(Level::Fatal, "test3"));

  Log::clearLevels();
}

TEST(FilterGroupTest, ApplyNoDefault) {
  Log::setLevel(Level::Debug);
  Log::setLevel(Level::Info, "test2");
  Log::setLevel(Level::Warning, "test3");
  EXPECT_TRUE(Log::isEnabled(Level::Debug, "test1"));
  EXPECT_TRUE(Log::isEnabled(Level::Info, "test2"));
  EXPECT_TRUE(Log::isEnabled(Level::Warning, "test3"));

  FilterGroup filter_group;
  filter_group.setLevel(Level::Info, "test1");
  filter_group.setLevel(Level::Warning, "test2");
  Log::applyFilterGroup(filter_group);
  EXPECT_FALSE(Log::isEnabled(Level::Debug, "test1"));
  EXPECT_FALSE(Log::isEnabled(Level::Info, "test2"));
  EXPECT_TRUE(Log::isEnabled(Level::Warning, "test3"));

  EXPECT_TRUE(Log::isEnabled(Level::Info, "test1"));
  EXPECT_TRUE(Log::isEnabled(Level::Warning, "test2"));
  EXPECT_TRUE(Log::isEnabled(Level::Debug, "test3"));

  Log::clearLevels();
}

TEST(FilterGroupTest, LoadBadFile) {
  FilterGroup filter_group;
  filter_group.setLevel(Level::Debug);
  filter_group.setLevel(Level::Info, "test1");
  filter_group.setLevel(Level::Warning, "test2");

  EXPECT_FALSE(filter_group.load("asdf"));
}

TEST(FilterGroupTest, LoadEmpty) {
  FilterGroup filter_group;
  filter_group.setLevel(Level::Debug);
  filter_group.setLevel(Level::Info, "test1");
  filter_group.setLevel(Level::Warning, "test2");

  std::stringstream stream("");
  EXPECT_TRUE(filter_group.load(stream));
  EXPECT_EQ(olp::porting::none, filter_group.getLevel());
  EXPECT_EQ(olp::porting::none, filter_group.getLevel("test1"));
  EXPECT_EQ(olp::porting::none, filter_group.getLevel("test2"));
}

TEST(FilterGroupTest, Load) {
  FilterGroup filter_group;
  filter_group.setLevel(Level::Debug);
  filter_group.setLevel(Level::Info, "test1");
  filter_group.setLevel(Level::Warning, "test2");

  std::stringstream stream(
      "   # this is a comment\n"
      "\t test2    :    ERRor   \n"
      "\n"
      "test3: off\n"
      ": info");
  EXPECT_TRUE(filter_group.load(stream));

  ASSERT_NE(olp::porting::none, filter_group.getLevel());
  EXPECT_EQ(Level::Info, *filter_group.getLevel());
  EXPECT_EQ(olp::porting::none, filter_group.getLevel("test1"));
  ASSERT_NE(olp::porting::none, filter_group.getLevel("test2"));
  EXPECT_EQ(Level::Error, *filter_group.getLevel("test2"));
  ASSERT_NE(olp::porting::none, filter_group.getLevel("test3"));
  EXPECT_EQ(Level::Off, *filter_group.getLevel("test3"));
}

TEST(FilterGroupTest, LoadBadSyntax) {
  FilterGroup filter_group;
  filter_group.setLevel(Level::Debug);
  filter_group.setLevel(Level::Info, "test1");
  filter_group.setLevel(Level::Warning, "test2");

  {
    std::stringstream stream("asdf");
    EXPECT_FALSE(filter_group.load(stream));
    EXPECT_EQ(olp::porting::none, filter_group.getLevel());
    EXPECT_EQ(olp::porting::none, filter_group.getLevel("test1"));
    EXPECT_EQ(olp::porting::none, filter_group.getLevel("test2"));
  }

  {
    std::stringstream stream("::");
    EXPECT_FALSE(filter_group.load(stream));
  }

  {
    std::stringstream stream("test1: asdf");
    EXPECT_FALSE(filter_group.load(stream));
  }

  {
    std::stringstream stream(": asdf");
    EXPECT_FALSE(filter_group.load(stream));
  }
}

}  // namespace
