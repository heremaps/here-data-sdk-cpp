/*
 * Copyright (C) 2023 HERE Europe B.V.
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

namespace olp {
namespace context {

/**
 * @brief Used only for the iOS environment to be informed that the application
 * is entering or exiting background.
 */
class EnterBackgroundSubscriber {
 public:
  /**
   * @brief Called when `Context` is entering background mode.
   */
  virtual void OnEnterBackground() = 0;

  /**
   * @brief Called when `Context` is exiting background mode.
   */
  virtual void OnExitBackground() = 0;
};

}  // namespace context
}  // namespace olp
