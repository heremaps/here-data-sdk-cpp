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

#include <fstream>
#include <string>
#include <utility>

#include <olp/core/CoreApi.h>
#include <olp/core/logging/Appender.h>
#include <olp/core/logging/MessageFormatter.h>

namespace olp {
namespace logging {
/**
 * @brief Appender for printing to a file.
 */
class CORE_API FileAppender : public IAppender {
 public:
  /**
   * @brief Constructs a file appender.
   * @param fileName The name of the file to write to.
   * @param append True to append to an existing file if it exists.
   * @param formatter The message formatter.
   */
  explicit FileAppender(
      const std::string& fileName, bool append = false,
      MessageFormatter formatter = MessageFormatter::createDefault());
  ~FileAppender() override;

  /**
   * @brief Returns whether or not the stream is opened and can be written to.
   * @return True if this is valid.
   */
  bool isValid() const;

  /**
   * @brief Gets the name of the file this was created with.
   * @return The file name.
   */
  inline const std::string& getFileName() const;

  /**
   * @brief Returns whether or not the file is being appended to.
   * @return True if appending to the file.
   */
  inline bool getAppendFile() const;

  /**
   * @brief Gets the message formatter.
   * @return The message formatter.
   */
  inline const MessageFormatter& getMessageFormatter() const;

  /**
   * @brief Sets the message formatter.
   * @param formatter The message formatter.
   */
  inline FileAppender& setMessageFormatter(MessageFormatter formatter);

  IAppender& append(const LogMessage& message) override;

 private:
  std::string m_fileName;
  bool m_appendFile;
  MessageFormatter m_formatter;
  std::ofstream m_stream;

  FileAppender(const FileAppender&) = delete;
  FileAppender& operator=(const FileAppender&) = delete;
};

inline const std::string& FileAppender::getFileName() const {
  return m_fileName;
}

inline bool FileAppender::getAppendFile() const { return m_appendFile; }

inline const MessageFormatter& FileAppender::getMessageFormatter() const {
  return m_formatter;
}

inline FileAppender& FileAppender::setMessageFormatter(
    MessageFormatter formatter) {
  m_formatter = std::move(formatter);
  return *this;
}

}  // namespace logging
}  // namespace olp
