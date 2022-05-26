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

#include <olp/core/logging/BufferAppender.h>

namespace olp {
namespace logging {
BufferAppender::BufferAppender(const uint16_t bufferSize)
    : m_buffer(bufferSize){ }

BufferAppender::~BufferAppender() {}

std::vector<LogMessage> BufferAppender::getLastMessages()
{
    std::vector<LogMessage> lastMessages(m_buffer.messageCount());

    std::lock_guard<std::mutex> _(m_mutex);
    std::copy(m_buffer.begin(), m_buffer.end(), lastMessages.begin());

    return std::move(lastMessages);
}

IAppender& BufferAppender::append(const LogMessage& message) {
    std::lock_guard<std::mutex> _(m_mutex);
    m_buffer.push_back(message);

  return *this;
}

}  // namespace logging
}  // namespace olp
