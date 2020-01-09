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

#include <gtest/gtest.h>
#include <memory>

#include <olp/core/logging/Configuration.h>
#include <olp/core/logging/Log.h>
#include "MockAppender.h"

namespace {

using namespace olp::logging;
using namespace testing;

TEST(ConfigurationTest, InvalidConfiguration) {
  Configuration configuration;
  EXPECT_FALSE(configuration.isValid());
  EXPECT_FALSE(Log::configure(configuration));
  OLP_SDK_LOG_TRACE("My tag", "My message");
}

TEST(ConfigurationTest, DefaultConfiguration) {
  Configuration configuration = Configuration::createDefault();
  EXPECT_TRUE(configuration.isValid());
  EXPECT_TRUE(Log::configure(configuration));
  OLP_SDK_LOG_TRACE("My tag", "My message");
}

TEST(ConfigurationTest, MockAppender) {
  Configuration configuration;
  configuration.addAppender(std::make_shared<MockAppender>());
  EXPECT_TRUE(configuration.isValid());
  EXPECT_TRUE(Log::configure(configuration));
  OLP_SDK_LOG_TRACE("My tag", "My message");
}

}  // namespace
