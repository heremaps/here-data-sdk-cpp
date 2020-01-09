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

#ifndef OLP_SDK_LOGGING_DISABLED
#define OLP_SDK_LOGGING_DISABLED  // disable all logging
#endif

#include <gtest/gtest.h>
#include <memory>
#include <string>

#include <olp/core/logging/Configuration.h>
#include <olp/core/logging/Log.h>
#include "MockAppender.h"

namespace {

using namespace olp::logging;
using namespace testing;

TEST(DisabledLoggingTest, LoggingDisabledDefined) {
  auto appender = std::make_shared<MockAppender>();
  Configuration configuration;
  configuration.addAppender(appender);
  EXPECT_TRUE(Log::configure(configuration));
  Log::setLevel(Level::Trace);

  // Log levels disabled by the flag
  OLP_SDK_LOG_INFO("", "No stream");
  OLP_SDK_LOG_TRACE("trace", "Trace message");
  OLP_SDK_LOG_DEBUG("debug", "Debug message");
  OLP_SDK_LOG_INFO("info", "Info message");
  OLP_SDK_LOG_WARNING("warning", "Warning message");
  OLP_SDK_LOG_ERROR("error", "Error message");

  // Log levels insuppressible by the flag
  OLP_SDK_LOG_FATAL("fatal", "Fatal message");
  OLP_SDK_LOG_CRITICAL_INFO("info", "Critical info message");
  OLP_SDK_LOG_CRITICAL_WARNING("warning", "Critical warning message");
  OLP_SDK_LOG_CRITICAL_ERROR("error", "Critical error message");

  ASSERT_EQ(4U, appender->messages_.size());
}

}  // namespace
