/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include "MockAppender.h"

namespace testing {

using namespace olp::logging;

IAppender& MockAppender::append(const LogMessage& message) {
  MockAppender::MessageData message_data;
  message_data.level_ = message.level;
  message_data.tag_ = message.tag;
  message_data.message_ = message.message;
  message_data.file_ = message.file;
  message_data.line_ = message.line;
  message_data.function_ = message.function;
  messages_.emplace_back(message_data);

  return *this;
}

}  // namespace testing
