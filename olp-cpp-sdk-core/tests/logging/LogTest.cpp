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
#include <sstream>
#include <string>
#include <vector>

#include <olp/core/logging/Configuration.h>
#include <olp/core/logging/ConsoleAppender.h>
#include <olp/core/logging/FileAppender.h>
#include <olp/core/logging/Log.h>
#include "MockAppender.h"

namespace {

TEST(LogTest, Levels) {
  EXPECT_TRUE(olp::logging::Log::configure(
      olp::logging::Configuration::createDefault()));

  olp::logging::Log::setLevel(olp::logging::Level::Info);
  EXPECT_EQ(olp::logging::Level::Info, olp::logging::Log::getLevel());
  EXPECT_EQ(olp::logging::Level::Info, olp::logging::Log::getLevel(""));
  EXPECT_TRUE(olp::logging::Log::isEnabled(olp::logging::Level::Info));
  EXPECT_FALSE(olp::logging::Log::isEnabled(olp::logging::Level::Debug));

  olp::logging::Log::setLevel(olp::logging::Level::Debug, "test1");
  olp::logging::Log::setLevel(olp::logging::Level::Warning, "test2");

  EXPECT_EQ(olp::logging::Level::Debug, olp::logging::Log::getLevel("test1"));
  EXPECT_EQ(olp::logging::Level::Warning, olp::logging::Log::getLevel("test2"));

  EXPECT_FALSE(olp::logging::Log::isEnabled(olp::logging::Level::Debug));
  EXPECT_TRUE(
      olp::logging::Log::isEnabled(olp::logging::Level::Debug, "test1"));

  EXPECT_FALSE(
      olp::logging::Log::isEnabled(olp::logging::Level::Debug, "test2"));
  EXPECT_FALSE(
      olp::logging::Log::isEnabled(olp::logging::Level::Info, "test2"));
  EXPECT_TRUE(
      olp::logging::Log::isEnabled(olp::logging::Level::Warning, "test2"));

  EXPECT_TRUE(
      olp::logging::Log::isEnabled(olp::logging::Level::Warning, "test3"));
  EXPECT_FALSE(
      olp::logging::Log::isEnabled(olp::logging::Level::Debug, "test3"));

  olp::logging::Log::clearLevel("test2");
  EXPECT_EQ(olp::logging::Level::Debug, olp::logging::Log::getLevel("test1"));
  EXPECT_EQ(boost::none, olp::logging::Log::getLevel("test2"));
  EXPECT_TRUE(
      olp::logging::Log::isEnabled(olp::logging::Level::Warning, "test2"));
  EXPECT_FALSE(
      olp::logging::Log::isEnabled(olp::logging::Level::Debug, "test2"));

  olp::logging::Log::clearLevels();
  EXPECT_EQ(boost::none, olp::logging::Log::getLevel("test1"));
  EXPECT_EQ(boost::none, olp::logging::Log::getLevel("test2"));
  EXPECT_TRUE(
      olp::logging::Log::isEnabled(olp::logging::Level::Warning, "test1"));
  EXPECT_FALSE(
      olp::logging::Log::isEnabled(olp::logging::Level::Debug, "test1"));
}

TEST(LogTest, DifferentLevelsForDifferentAppenders) {
  auto appender1 = std::make_shared<testing::MockAppender>();
  auto appender2 = std::make_shared<testing::MockAppender>();

  {
    olp::logging::Configuration configuration;
    configuration.addAppender(appender1, olp::logging::Level::Trace);
    EXPECT_TRUE(olp::logging::Log::configure(configuration));
  }
  {
    // Add a new appender to the existing configuration.
    olp::logging::Configuration configuration =
        olp::logging::Log::getConfiguration();
    configuration.addAppender(appender2, olp::logging::Level::Info);
    EXPECT_TRUE(olp::logging::Log::configure(configuration));
  }
  olp::logging::Log::setLevel(olp::logging::Level::Trace);

  OLP_SDK_LOG_TRACE("trace", "Trace "
                                 << "message");
  OLP_SDK_LOG_INFO("info", "Info "
                               << "message");

  ASSERT_EQ(2U, appender1->messages_.size());
  EXPECT_EQ(olp::logging::Level::Trace, appender1->messages_[0].level_);
  EXPECT_EQ("trace", appender1->messages_[0].tag_);
  EXPECT_EQ("Trace message", appender1->messages_[0].message_);

  ASSERT_EQ(1U, appender2->messages_.size());
  EXPECT_EQ(olp::logging::Level::Info, appender2->messages_[0].level_);
  EXPECT_EQ("info", appender2->messages_[0].tag_);
  EXPECT_EQ("Info message", appender2->messages_[0].message_);

  // if messages are filtered out by tag they are not supposed to be appended
  // (regardless of appender's log level configuration)
  olp::logging::Log::setLevel(olp::logging::Level::Error, "test");
  OLP_SDK_LOG_WARNING("test", "Test "
                                  << "message");

  ASSERT_EQ(2U, appender1->messages_.size());
  ASSERT_EQ(1U, appender2->messages_.size());
}

TEST(LogTest, DifferentLevelsForConsoleAndFileLogging) {
  olp::logging::MessageFormatter formatter(
      {olp::logging::MessageFormatter::Element(
           olp::logging::MessageFormatter::ElementType::Level, "%s "),
       olp::logging::MessageFormatter::Element(
           olp::logging::MessageFormatter::ElementType::Tag, "%s - "),
       olp::logging::MessageFormatter::Element(
           olp::logging::MessageFormatter::ElementType::Message)});

  auto console_appender =
      std::make_shared<olp::logging::ConsoleAppender>(formatter);
  auto file_appender = std::make_shared<olp::logging::FileAppender>(
      "test.txt", false, formatter);

  ASSERT_TRUE(file_appender->isValid());
  EXPECT_EQ("test.txt", file_appender->getFileName());

  {
    olp::logging::Configuration configuration;
    configuration.addAppender(console_appender, olp::logging::Level::Warning);
    EXPECT_TRUE(olp::logging::Log::configure(configuration));
  }
  {
    // Add a new appender to the existing configuration.
    olp::logging::Configuration configuration =
        olp::logging::Log::getConfiguration();
    configuration.addAppender(file_appender, olp::logging::Level::Trace);
    EXPECT_TRUE(olp::logging::Log::configure(configuration));
  }
  olp::logging::Log::setLevel(olp::logging::Level::Trace);

  // redirecting cout to stringstream
  std::stringstream string_stream;
  auto default_cout_buffer = std::cout.rdbuf(string_stream.rdbuf());

  OLP_SDK_LOG_INFO("info", "Info "
                               << "message");
  OLP_SDK_LOG_TRACE("trace", "Trace "
                                 << "message");
  OLP_SDK_LOG_WARNING("warn", "Warning "
                                  << "message");
  OLP_SDK_LOG_ERROR("err", "Error "
                               << "message");
  OLP_SDK_LOG_FATAL("fatal", "Fatal "
                                 << "message");

  // resetting cout
  std::cout.rdbuf(default_cout_buffer);

  // Clear out configuration so the file file handles are closed.
  olp::logging::Log::configure(olp::logging::Configuration::createDefault());
  file_appender.reset();

  // checking console appender's output
  {
    std::vector<std::string> lines;
    const unsigned int bufferSize = 256;
    char buffer[bufferSize];
    for (;;) {
      string_stream.getline(buffer, bufferSize);
      if (!string_stream.good()) {
        break;
      }
      lines.emplace_back(buffer);
    }

    std::vector<std::string> expected_lines = {"[WARN] warn - Warning message",
                                               "[ERROR] err - Error message",
                                               "[FATAL] fatal - Fatal message"};

    EXPECT_EQ(expected_lines, lines);
  }

  // checking file appender's output
  {
    std::ifstream stream("test.txt");
    EXPECT_TRUE(stream.good());

    std::vector<std::string> lines;
    const unsigned int buffer_size = 256;
    char buffer[buffer_size];
    for (;;) {
      stream.getline(buffer, buffer_size);
      if (!stream.good()) {
        break;
      }
      lines.emplace_back(buffer);
    }
    stream.close();

    std::vector<std::string> expected_lines = {
        "[INFO] info - Info message", "[TRACE] trace - Trace message",
        "[WARN] warn - Warning message", "[ERROR] err - Error message",
        "[FATAL] fatal - Fatal message"};

    EXPECT_EQ(expected_lines, lines);
    EXPECT_EQ(0, unlink("test.txt"));
  }
}

TEST(LogTest, LogToStream) {
  auto appender = std::make_shared<testing::MockAppender>();
  olp::logging::Configuration configuration;
  configuration.addAppender(appender);
  EXPECT_TRUE(olp::logging::Log::configure(configuration));
  olp::logging::Log::setLevel(olp::logging::Level::Trace);

  OLP_SDK_LOG_INFO("", "No stream");
  OLP_SDK_LOG_TRACE("trace", "Trace "
                                 << "message");
  OLP_SDK_LOG_DEBUG("debug", "Debug "
                                 << "message");
  OLP_SDK_LOG_INFO("info", "Info "
                               << "message");
  OLP_SDK_LOG_WARNING("warning", "Warning "
                                     << "message");
  OLP_SDK_LOG_ERROR("error", "Error "
                                 << "message");
  OLP_SDK_LOG_FATAL("fatal", "Fatal "
                                 << "message");

  ASSERT_EQ(7U, appender->messages_.size());

  unsigned int index = 0;
  EXPECT_EQ(olp::logging::Level::Info, appender->messages_[index].level_);
  EXPECT_EQ("", appender->messages_[index].tag_);
  EXPECT_EQ("No stream", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogStream_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Trace, appender->messages_[index].level_);
  EXPECT_EQ("trace", appender->messages_[index].tag_);
  EXPECT_EQ("Trace message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogStream_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Debug, appender->messages_[index].level_);
  EXPECT_EQ("debug", appender->messages_[index].tag_);
  EXPECT_EQ("Debug message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogStream_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Info, appender->messages_[index].level_);
  EXPECT_EQ("info", appender->messages_[index].tag_);
  EXPECT_EQ("Info message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogStream_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Warning, appender->messages_[index].level_);
  EXPECT_EQ("warning", appender->messages_[index].tag_);
  EXPECT_EQ("Warning message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogStream_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Error, appender->messages_[index].level_);
  EXPECT_EQ("error", appender->messages_[index].tag_);
  EXPECT_EQ("Error message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogStream_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Fatal, appender->messages_[index].level_);
  EXPECT_EQ("fatal", appender->messages_[index].tag_);
  EXPECT_EQ("Fatal message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogStream_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;
}

TEST(LogTest, LogFormat) {
  auto appender = std::make_shared<testing::MockAppender>();
  olp::logging::Configuration configuration;
  configuration.addAppender(appender);
  EXPECT_TRUE(olp::logging::Log::configure(configuration));
  olp::logging::Log::setLevel(olp::logging::Level::Trace);

  OLP_SDK_LOG_INFO_F("", "No format args");
  OLP_SDK_LOG_TRACE_F("trace", "%s %s", "Trace", "message");
  OLP_SDK_LOG_DEBUG_F("debug", "%s %s", "Debug", "message");
  OLP_SDK_LOG_INFO_F("info", "%s %s", "Info", "message");
  OLP_SDK_LOG_WARNING_F("warning", "%s %s", "Warning", "message");
  OLP_SDK_LOG_ERROR_F("error", "%s %s", "Error", "message");
  OLP_SDK_LOG_FATAL_F("fatal", "%s %s", "Fatal", "message");

  ASSERT_EQ(7U, appender->messages_.size());

  unsigned int index = 0;
  EXPECT_EQ(olp::logging::Level::Info, appender->messages_[index].level_);
  EXPECT_EQ("", appender->messages_[index].tag_);
  EXPECT_EQ("No format args", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogFormat_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Trace, appender->messages_[index].level_);
  EXPECT_EQ("trace", appender->messages_[index].tag_);
  EXPECT_EQ("Trace message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogFormat_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Debug, appender->messages_[index].level_);
  EXPECT_EQ("debug", appender->messages_[index].tag_);
  EXPECT_EQ("Debug message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogFormat_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Info, appender->messages_[index].level_);
  EXPECT_EQ("info", appender->messages_[index].tag_);
  EXPECT_EQ("Info message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogFormat_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Warning, appender->messages_[index].level_);
  EXPECT_EQ("warning", appender->messages_[index].tag_);
  EXPECT_EQ("Warning message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogFormat_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Error, appender->messages_[index].level_);
  EXPECT_EQ("error", appender->messages_[index].tag_);
  EXPECT_EQ("Error message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogFormat_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Fatal, appender->messages_[index].level_);
  EXPECT_EQ("fatal", appender->messages_[index].tag_);
  EXPECT_EQ("Fatal message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogFormat_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;
}

TEST(LogTest, LogLimits) {
  auto appender = std::make_shared<testing::MockAppender>();
  olp::logging::Configuration configuration;
  configuration.addAppender(appender);
  EXPECT_TRUE(olp::logging::Log::configure(configuration));
  olp::logging::Log::setLevel(olp::logging::Level::Info);

  OLP_SDK_LOG_TRACE("trace", "Trace "
                                 << "message");
  OLP_SDK_LOG_DEBUG("debug", "Debug "
                                 << "message");
  OLP_SDK_LOG_INFO("info", "Info "
                               << "message");
  OLP_SDK_LOG_WARNING("warning", "Warning "
                                     << "message");
  OLP_SDK_LOG_ERROR("error", "Error "
                                 << "message");
  OLP_SDK_LOG_FATAL("fatal", "Fatal "
                                 << "message");

  ASSERT_EQ(4U, appender->messages_.size());

  unsigned int index = 0;
  EXPECT_EQ(olp::logging::Level::Info, appender->messages_[index].level_);
  EXPECT_EQ("info", appender->messages_[index].tag_);
  EXPECT_EQ("Info message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogLimits_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Warning, appender->messages_[index].level_);
  EXPECT_EQ("warning", appender->messages_[index].tag_);
  EXPECT_EQ("Warning message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogLimits_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Error, appender->messages_[index].level_);
  EXPECT_EQ("error", appender->messages_[index].tag_);
  EXPECT_EQ("Error message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogLimits_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Fatal, appender->messages_[index].level_);
  EXPECT_EQ("fatal", appender->messages_[index].tag_);
  EXPECT_EQ("Fatal message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogLimits_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;
}

TEST(LogTest, LogOverrideLimits) {
  auto appender = std::make_shared<testing::MockAppender>();
  olp::logging::Configuration configuration;
  configuration.addAppender(appender);
  EXPECT_TRUE(olp::logging::Log::configure(configuration));
  olp::logging::Log::setLevel(olp::logging::Level::Trace);
  olp::logging::Log::clearLevels();

  OLP_SDK_LOG_TRACE("test", "Trace "
                                << "message");
  OLP_SDK_LOG_DEBUG("test", "Debug "
                                << "message");
  OLP_SDK_LOG_INFO("test", "Info "
                               << "message");
  OLP_SDK_LOG_WARNING("test", "Warning "
                                  << "message");
  OLP_SDK_LOG_ERROR("test", "Error "
                                << "message");
  OLP_SDK_LOG_FATAL("test", "Fatal "
                                << "message");

  EXPECT_EQ(6U, appender->messages_.size());
  appender->messages_.clear();

  olp::logging::Log::setLevel(olp::logging::Level::Info, "test");
  OLP_SDK_LOG_TRACE("test", "Trace "
                                << "message");
  OLP_SDK_LOG_DEBUG("test", "Debug "
                                << "message");
  OLP_SDK_LOG_INFO("test", "Info "
                               << "message");
  OLP_SDK_LOG_WARNING("test", "Warning "
                                  << "message");
  OLP_SDK_LOG_ERROR("test", "Error "
                                << "message");
  OLP_SDK_LOG_FATAL("test", "Fatal "
                                << "message");

  ASSERT_EQ(4U, appender->messages_.size());

  unsigned int index = 0;
  EXPECT_EQ(olp::logging::Level::Info, appender->messages_[index].level_);
  EXPECT_EQ("test", appender->messages_[index].tag_);
  EXPECT_EQ("Info message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogOverrideLimits_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Warning, appender->messages_[index].level_);
  EXPECT_EQ("test", appender->messages_[index].tag_);
  EXPECT_EQ("Warning message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogOverrideLimits_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Error, appender->messages_[index].level_);
  EXPECT_EQ("test", appender->messages_[index].tag_);
  EXPECT_EQ("Error message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogOverrideLimits_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Fatal, appender->messages_[index].level_);
  EXPECT_EQ("test", appender->messages_[index].tag_);
  EXPECT_EQ("Fatal message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogOverrideLimits_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  olp::logging::Log::clearLevels();
}

TEST(LogTest, LogLevelOff) {
  auto appender = std::make_shared<testing::MockAppender>();
  olp::logging::Configuration configuration;
  configuration.addAppender(appender);
  EXPECT_TRUE(olp::logging::Log::configure(configuration));
  olp::logging::Log::setLevel(olp::logging::Level::Off);

  OLP_SDK_LOG_TRACE("trace", "Trace "
                                 << "message");
  OLP_SDK_LOG_DEBUG("debug", "Debug "
                                 << "message");
  OLP_SDK_LOG_INFO("info", "Info "
                               << "message");
  OLP_SDK_LOG_WARNING("warning", "Warning "
                                     << "message");
  OLP_SDK_LOG_ERROR("error", "Error "
                                 << "message");
  OLP_SDK_LOG_FATAL("fatal", "Fatal "
                                 << "message");

  OLP_SDK_LOG_CRITICAL_INFO("info", "Critical info "
                                        << "message");
  OLP_SDK_LOG_CRITICAL_WARNING("warning", "Critical warning "
                                              << "message");
  OLP_SDK_LOG_CRITICAL_ERROR("error", "Critical error "
                                          << "message");

  ASSERT_EQ(4U, appender->messages_.size());

  unsigned index = 0;
  EXPECT_EQ(olp::logging::Level::Fatal, appender->messages_[index].level_);
  EXPECT_EQ("fatal", appender->messages_[index].tag_);
  EXPECT_EQ("Fatal message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogOff_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Info, appender->messages_[index].level_);
  EXPECT_EQ("info", appender->messages_[index].tag_);
  EXPECT_EQ("Critical info message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogOff_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Warning, appender->messages_[index].level_);
  EXPECT_EQ("warning", appender->messages_[index].tag_);
  EXPECT_EQ("Critical warning message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogOff_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;

  EXPECT_EQ(olp::logging::Level::Error, appender->messages_[index].level_);
  EXPECT_EQ("error", appender->messages_[index].tag_);
  EXPECT_EQ("Critical error message", appender->messages_[index].message_);
  EXPECT_NE(std::string::npos,
            appender->messages_[index].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender->messages_[index].line_);
  EXPECT_NE("LogTest_LogOff_Test::TestBody()",
            appender->messages_[index].function_);
  ++index;
}

TEST(LogTest, ReConfigure) {
  auto appender1 = std::make_shared<testing::MockAppender>();
  auto appender2 = std::make_shared<testing::MockAppender>();
  {
    olp::logging::Configuration configuration;
    configuration.addAppender(appender1);
    EXPECT_TRUE(olp::logging::Log::configure(configuration));
  }
  {
    // Add a new appender to the existing configuration.
    olp::logging::Configuration configuration =
        olp::logging::Log::getConfiguration();
    configuration.addAppender(appender2);
    EXPECT_TRUE(olp::logging::Log::configure(configuration));
  }
  olp::logging::Log::setLevel(olp::logging::Level::Trace);

  OLP_SDK_LOG_TRACE("trace", "Trace "
                                 << "message");

  ASSERT_EQ(1U, appender1->messages_.size());
  EXPECT_EQ(olp::logging::Level::Trace, appender1->messages_[0].level_);
  EXPECT_EQ("trace", appender1->messages_[0].tag_);
  EXPECT_EQ("Trace message", appender1->messages_[0].message_);
  EXPECT_NE(std::string::npos,
            appender1->messages_[0].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender1->messages_[0].line_);
  EXPECT_NE("LogTest_LogStream_Test::TestBody()",
            appender1->messages_[0].function_);

  ASSERT_EQ(1U, appender2->messages_.size());
  EXPECT_EQ(olp::logging::Level::Trace, appender2->messages_[0].level_);
  EXPECT_EQ("trace", appender2->messages_[0].tag_);
  EXPECT_EQ("Trace message", appender2->messages_[0].message_);
  EXPECT_NE(std::string::npos,
            appender2->messages_[0].file_.rfind("LogTest.cpp"));
  EXPECT_LT(0U, appender2->messages_[0].line_);
  EXPECT_NE("LogTest_LogStream_Test::TestBody()",
            appender2->messages_[0].function_);
}

}  // namespace
