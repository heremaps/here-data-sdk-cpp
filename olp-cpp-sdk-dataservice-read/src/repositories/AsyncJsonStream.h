/*
 * Copyright (C) 2023-2025 HERE Europe B.V.
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

#include <condition_variable>
#include <mutex>
#include <vector>

#include <olp/core/client/ApiError.h>
#include <olp/core/porting/optional.h>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

/// Json byte stream class. Implements rapidjson input stream concept.
class RapidJsonByteStream {
 public:
  typedef char Ch;

  /// Read the current character from stream without moving the read cursor.
  Ch Peek();

  /// Read the current character from stream and moving the read cursor to next
  /// character.
  Ch Take();

  /// Get the current read cursor.
  size_t Tell() const;

  /// Not needed for reading.
  char* PutBegin();
  void Put(char);
  void Flush();
  size_t PutEnd(char*);

  bool ReadEmpty() const;
  bool WriteEmpty() const;

  void AppendContent(const char* content, size_t length);

 private:
  void SwapBuffers();

  mutable std::mutex mutex_;
  std::vector<char> read_buffer_;       // Current buffer
  std::vector<char> write_buffer_;      // Current buffer
  size_t full_count_{0};                // Bytes read from the buffer
  size_t count_{0};                     // Bytes read from the buffer
  mutable std::condition_variable cv_;  // Condition for next portion of content
};

class AsyncJsonStream {
 public:
  AsyncJsonStream();

  std::shared_ptr<RapidJsonByteStream> GetCurrentStream() const;

  void AppendContent(const char* content, size_t length);

  void ResetStream(const char* content, size_t length);

  void CloseStream(porting::optional<client::ApiError> error);

  porting::optional<client::ApiError> GetError() const;

  bool IsClosed() const;

 private:
  mutable std::mutex mutex_;
  std::shared_ptr<RapidJsonByteStream> current_stream_;
  porting::optional<client::ApiError> error_;
  bool closed_;
};

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
