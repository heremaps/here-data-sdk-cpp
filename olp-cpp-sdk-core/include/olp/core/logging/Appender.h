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

#pragma once

#include <olp/core/logging/LogMessage.h>

namespace olp {
namespace logging {
/**
 * @brief Abstract base class for an appender, which appends a message to the
 * log.
 *
 * Subclasses should derive from Appender instead of IAppender in order for the
 * type to be automatically registered.
 */
class CORE_API IAppender {
 public:
  virtual ~IAppender() = default;

  /**
   * @brief Appends a message using char strings.
   * @param message The message to append.
   */
  virtual IAppender& append(const LogMessage& message) = 0;
};

}  // namespace logging
}  // namespace olp
