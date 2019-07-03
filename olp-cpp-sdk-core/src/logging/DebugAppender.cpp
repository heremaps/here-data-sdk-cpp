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

#include <olp/core/logging/DebugAppender.h>
#include <olp/core/logging/Format.h>
#include <olp/core/porting/platform.h>

#ifdef PORTING_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace olp {
namespace logging {
#ifdef PORTING_PLATFORM_WINDOWS

bool DebugAppender::isImplemented() { return true; }

void DebugAppender::append(const LogMessage& message) {
  const char* prefix = nullptr;
  bool showLocation = false;
  switch (message.level) {
    case Level::Trace:
      prefix = "trace: ";
      break;
    case Level::Debug:
      prefix = "debug: ";
      break;
    case Level::Info:
      prefix = "info: ";
      break;
    case Level::Warning:
      prefix = "warning: ";
      showLocation = true;
      break;
    case Level::Error:
      prefix = "error: ";
      showLocation = true;
      break;
    case Level::Fatal:
      prefix = "fatal: ";
      showLocation = true;
      break;
    default:
      return;
  }

  if (showLocation) {
    FormatBuffer buffer;
    OutputDebugStringA(buffer.format("%s(%d) : %s(): ", message.file,
                                     message.line, message.function));
  }

  if (message.tag && *message.tag) {
    OutputDebugStringA(message.tag);
    OutputDebugStringA(" ");
  }

  OutputDebugStringA(prefix);
  OutputDebugStringA(message.message);
  OutputDebugStringA("\n");
}

#else

bool DebugAppender::isImplemented() { return false; }

void DebugAppender::append(const LogMessage&) {}

#endif  // LOG_PLATFORM_WINDOWS

}  // namespace logging
}  // namespace olp
