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

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include <olp/core/logging/Configuration.h>
#include <olp/core/logging/BufferAppender.h>
#include <olp/core/logging/Log.h>

#ifdef PORTING_PLATFORM_WINDOWS
#include <stdio.h>
#define unlink _unlink
#else
#include <unistd.h>
#endif

namespace {

namespace logging = olp::logging;
constexpr uint16_t BUFFER_SIZE = 5;
constexpr char TAG[] = "BufferAppenderTest";

TEST(BufferAppenderTest, Default) {
    {
      SCOPED_TRACE("Append");
      auto appender =
          std::make_shared<logging::BufferAppender>(BUFFER_SIZE);

      logging::Configuration configuration{};
      configuration.addAppender(appender);
      logging::Log::configure(configuration);
      logging::Log::setLevel(logging::Level::Info);

      OLP_SDK_LOG_INFO(TAG, "test 0");
      OLP_SDK_LOG_INFO(TAG, "test 1");
      OLP_SDK_LOG_INFO(TAG, "test 2");

      logging::Log::configure(logging::Configuration::createDefault());
      auto lastMessages=appender->getLastMessages();

      EXPECT_EQ(lastMessages.size(), 3);
      EXPECT_EQ(std::string(lastMessages[0].message), std::string("test 0"));
      EXPECT_EQ(std::string(lastMessages[2].message), std::string("test 2"));
  }

  {
    SCOPED_TRACE("Circulate");
    auto appender =
        std::make_shared<logging::BufferAppender>(BUFFER_SIZE);

    logging::Configuration configuration{};
    configuration.addAppender(appender);
    logging::Log::configure(configuration);
    logging::Log::setLevel(logging::Level::Info);

    OLP_SDK_LOG_INFO(TAG, "test 0");
    OLP_SDK_LOG_INFO(TAG, "test 1");
    OLP_SDK_LOG_INFO(TAG, "test 2");
    OLP_SDK_LOG_INFO(TAG, "test 3");
    OLP_SDK_LOG_INFO(TAG, "test 4");
    OLP_SDK_LOG_INFO(TAG, "test 5");
    OLP_SDK_LOG_INFO(TAG, "test 6");

    logging::Log::configure(logging::Configuration::createDefault());
    auto lastMessages=appender->getLastMessages();

    EXPECT_EQ(lastMessages.size(), BUFFER_SIZE);
    EXPECT_EQ(std::string(lastMessages[0].message), std::string("test 2"));
    EXPECT_EQ(std::string(lastMessages[BUFFER_SIZE-1].message), std::string("test 6"));
  }
}
}  // namespace
