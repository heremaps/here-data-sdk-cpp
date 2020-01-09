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
#include <string>
#include <vector>

#include <olp/core/logging/Configuration.h>
#include <olp/core/logging/FileAppender.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/platform.h>

#ifdef PORTING_PLATFORM_WINDOWS
#include <stdio.h>
#define unlink _unlink
#else
#include <unistd.h>
#endif

namespace {

using namespace olp::logging;
using namespace testing;

TEST(FileAppenderTest, Default) {
  {
    SCOPED_TRACE("Create appender");
    auto appender = std::make_shared<FileAppender>("test.txt");
    ASSERT_TRUE(appender->isValid());
    EXPECT_EQ("test.txt", appender->getFileName());

    Configuration configuration{};
    configuration.addAppender(appender);
    Log::configure(configuration);
    Log::setLevel(Level::Info);

    OLP_SDK_LOG_INFO("test", "test 1");
    OLP_SDK_LOG_WARNING("test", "test 2");

    Log::configure(Configuration::createDefault());
  }

  {
    SCOPED_TRACE("Check the log file content");
    std::ifstream stream("test.txt");
    EXPECT_TRUE(stream.good());

    std::vector<std::string> lines;
    const unsigned int buffer_size = 1024;
    char buffer[buffer_size];
    while (true) {
      stream.getline(buffer, buffer_size);
      if (!stream.good()) {
        break;
      }
      lines.emplace_back(buffer);
    }
    stream.close();

    std::vector<std::string> expected_lines = {"[INFO] test - test 1",
                                               "[WARN] test - test 2"};
    EXPECT_EQ(expected_lines, lines);
    EXPECT_EQ(0, unlink("test.txt"));
  }
}

TEST(FileAppenderTest, NonExistingFile) {
  auto appender = std::make_shared<FileAppender>("asdf/foo/bar");
  EXPECT_FALSE(appender->isValid());
}

TEST(FileAppenderTest, Append) {
  {
    SCOPED_TRACE("Create appender");
    auto appender = std::make_shared<FileAppender>("test.txt", true);
    ASSERT_TRUE(appender->isValid());
    EXPECT_EQ("test.txt", appender->getFileName());
    EXPECT_TRUE(appender->getAppendFile());

    Configuration configuration{};
    configuration.addAppender(appender);
    Log::configure(configuration);
    Log::setLevel(Level::Info);

    OLP_SDK_LOG_INFO("test", "test 1");
    OLP_SDK_LOG_WARNING("test", "test 2");

    Log::configure(Configuration::createDefault());
  }

  {
    SCOPED_TRACE("Re-create appender");
    auto appender = std::make_shared<FileAppender>("test.txt", true);
    EXPECT_TRUE(appender->isValid());

    Configuration configuration{};
    configuration.addAppender(appender);
    Log::configure(configuration);

    OLP_SDK_LOG_ERROR("test", "test 3");
    OLP_SDK_LOG_FATAL("test", "test 4");

    Log::configure(Configuration::createDefault());
  }

  {
    SCOPED_TRACE("Check the log file");
    std::ifstream stream("test.txt");
    EXPECT_TRUE(stream.good());

    std::vector<std::string> lines;
    const unsigned int buffer_size = 1024;
    char buffer[buffer_size];
    while (true) {
      stream.getline(buffer, buffer_size);
      if (!stream.good()) {
        break;
      }
      lines.emplace_back(buffer);
    }
    stream.close();

    std::vector<std::string> expected_lines = {
        "[INFO] test - test 1", "[WARN] test - test 2", "[ERROR] test - test 3",
        "[FATAL] test - test 4"};
    EXPECT_EQ(expected_lines, lines);
    EXPECT_EQ(0, unlink("test.txt"));
  }
}

}  // namespace
