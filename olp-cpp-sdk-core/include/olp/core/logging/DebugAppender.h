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

#include <olp/core/CoreApi.h>
#include <olp/core/logging/Appender.h>

namespace olp {
namespace logging {
/**
 * @brief Appender for printing to the debug output.
 */
class CORE_API DebugAppender : public IAppender {
 public:
  /**
   * @brief Returns whether or not the debug appender is implemented for this
   * platform.
   * @return Whether or not this is implemented.
   */
  static bool isImplemented();

  IAppender& append(const LogMessage& message) override;
};

}  // namespace logging
}  // namespace olp
