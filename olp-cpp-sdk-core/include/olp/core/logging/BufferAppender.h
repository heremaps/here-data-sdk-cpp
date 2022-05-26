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

#pragma once

#include <vector>
#include "CircularBuffer.h"
#include <mutex>

#include <olp/core/CoreApi.h>
#include <olp/core/logging/Appender.h>

namespace olp {
namespace logging {
/**
 * @brief An appender that stores messages in circular buffer.
 */
class CORE_API BufferAppender : public IAppender {
 public:
  /**
   * @brief Creates a `BufferAppender` instance.
   *
   * @param bufferSize Size of a background circular buffer,
   * basically a maximum number of available last messages.
   */
  BufferAppender(const uint16_t bufferSize);
  ~BufferAppender() override;

  /**
   * @brief Returns current contents of messages buffer.
   *
   * @return Vector of last available messages.
   */
  std::vector<LogMessage> getLastMessages();

  /// @copydoc IAppender::append( const LogMessage& message )
  IAppender& append(const LogMessage& message) override;

 private:
  mutable std::mutex m_mutex;
  CircularBuffer<LogMessage> m_buffer;

  BufferAppender(const BufferAppender&) = delete;
  BufferAppender& operator=(const BufferAppender&) = delete;
};

}  // namespace logging
}  // namespace olp
