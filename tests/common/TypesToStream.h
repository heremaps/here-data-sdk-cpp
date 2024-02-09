/*
 * Copyright (C) 2024 HERE Europe B.V.
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

#include <string>
#include <utility>

#include <olp/core/logging/Log.h>

namespace std {
inline std::ostream& operator<<(
    std::ostream& os, const std::pair<std::string, std::string>& pair) {
  return os << "pair< first:'" << pair.first << "'; second: '" << pair.second
            << "' >";
}
}  // namespace std

namespace olp {
namespace logging {
inline std::ostream& operator<<(std::ostream& os, const Level level) {
  switch (level) {
    case Level::Trace:
      return os << "Trace";
    case Level::Debug:
      return os << "Debug";
    case Level::Info:
      return os << "Info";
    case Level::Warning:
      return os << "Warning";
    case Level::Error:
      return os << "Error";
    case Level::Fatal:
      return os << "Fatal";
    case Level::Off:
      return os << "Off";
    default:
      return os << "Unknown logging level";
  }
}
}  // namespace logging
}  // namespace olp
