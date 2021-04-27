/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#pragma once

#include <olp/core/CoreApi.h>
#include <olp/core/logging/Level.h>
#include <chrono>

namespace olp {
namespace logging {
/**
 * @brief Contains data used for a log message.
 */
struct CORE_API LogMessage {
  /// The log level.
  Level level{Level::Off};

  /// The tag for the log component.
  const char* tag{nullptr};

  /// The log message.
  const char* message{nullptr};

  /// The file that generated the message.
  const char* file{nullptr};

  /// The line in the file where the message was logged.
  unsigned int line{0};

  /// The function that generated the message.
  const char* function{nullptr};

  /// The fully qualified function that generated the message.
  const char* fullFunction{nullptr};

  /// The timestamp of the message.
  std::chrono::time_point<std::chrono::system_clock> time{};

  /// The thread ID for the thread that generated the message.
  unsigned long threadId{0};
};

}  // namespace logging
}  // namespace olp
