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

#include <utility>

#include <olp/core/CoreApi.h>
#include <olp/core/logging/Appender.h>
#include <olp/core/logging/MessageFormatter.h>

namespace olp {
namespace logging {
/**
 * @brief An appender that prints messages to the console.
 */
class CORE_API ConsoleAppender : public IAppender {
 public:
  /**
   * @brief Creates a `ConsoleAppender` instance with a message formatter.
   *
   * @param formatter The message formatter.
   */
  inline explicit ConsoleAppender(
      MessageFormatter formatter = MessageFormatter::createDefault());

  /**
   * @brief Gets the message formatter.
   *
   * @return The message formatter.
   */
  inline const MessageFormatter& getMessageFormatter() const;

  /**
   * @brief Sets the message formatter.
   *
   * @param formatter The message formatter.
   */
  inline ConsoleAppender& setMessageFormatter(MessageFormatter formatter);

  IAppender& append(const LogMessage& message) override;

 private:
  MessageFormatter m_formatter;
};

inline ConsoleAppender::ConsoleAppender(MessageFormatter formatter)
    : m_formatter(std::move(formatter)) {}

inline const MessageFormatter& ConsoleAppender::getMessageFormatter() const {
  return m_formatter;
}

inline ConsoleAppender& ConsoleAppender::setMessageFormatter(
    MessageFormatter formatter) {
  m_formatter = std::move(formatter);
  return *this;
}

}  // namespace logging
}  // namespace olp
