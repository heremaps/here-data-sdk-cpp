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

#include <olp/core/logging/FileAppender.h>
#include <olp/core/logging/Format.h>
#include <olp/core/porting/platform.h>
#include <string>

namespace olp {
namespace logging {
FileAppender::FileAppender(const std::string& fileName, bool append,
                           MessageFormatter formatter)
    : m_fileName(fileName),
      m_appendFile(append),
      m_formatter(std::move(formatter)) {
  std::ios_base::openmode mode = std::ios_base::out;
  if (append)
    mode |= std::ios_base::app;
  else
    mode |= std::ios_base::trunc;
  m_stream.open(fileName, mode);
}

FileAppender::~FileAppender() {}

bool FileAppender::isValid() const { return m_stream.good(); }

IAppender& FileAppender::append(const LogMessage& message) {
  if (!isValid()) return *this;

  m_stream << m_formatter.format(message) << std::endl;
  return *this;
}

}  // namespace logging
}  // namespace olp
