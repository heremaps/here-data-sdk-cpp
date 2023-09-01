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

#include "AsyncJsonStream.h"

#include <cstring>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

RapidJsonByteStream::Ch RapidJsonByteStream::Peek() const {
  std::unique_lock<std::mutex> lock(mutex_);
  cv_.wait(lock, [=]() { return !Empty(); });
  return buffer_[count_];
}

RapidJsonByteStream::Ch RapidJsonByteStream::Take() {
  std::unique_lock<std::mutex> lock(mutex_);
  cv_.wait(lock, [=]() { return !Empty(); });
  return buffer_[count_++];
}

size_t RapidJsonByteStream::Tell() const { return count_; }

// Not implemented
char* RapidJsonByteStream::PutBegin() { return 0; }
void RapidJsonByteStream::Put(char) {}
void RapidJsonByteStream::Flush() {}
size_t RapidJsonByteStream::PutEnd(char*) { return 0; }

bool RapidJsonByteStream::Empty() const { return count_ == buffer_.size(); }

void RapidJsonByteStream::AppendContent(const char* content, size_t length) {
  std::unique_lock<std::mutex> lock(mutex_);

  if (Empty()) {
    buffer_.resize(length);
    std::memcpy(buffer_.data(), content, length);
    count_ = 0;
  } else {
    const auto buffer_size = buffer_.size();
    buffer_.resize(buffer_size + length);
    std::memcpy(buffer_.data() + buffer_size, content, length);
  }

  cv_.notify_one();
}

AsyncJsonStream::AsyncJsonStream()
    : current_stream_(std::make_shared<RapidJsonByteStream>()),
      closed_{false} {}

std::shared_ptr<RapidJsonByteStream> AsyncJsonStream::GetCurrentStream() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return current_stream_;
}

void AsyncJsonStream::AppendContent(const char* content, size_t length) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (closed_) {
    return;
  }
  current_stream_->AppendContent(content, length);
}

void AsyncJsonStream::ResetStream(const char* content, size_t length) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (closed_) {
    return;
  }
  current_stream_->AppendContent("\0", 1);
  current_stream_ = std::make_shared<RapidJsonByteStream>();
  current_stream_->AppendContent(content, length);
}

void AsyncJsonStream::CloseStream(boost::optional<client::ApiError> error) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (closed_) {
    return;
  }
  current_stream_->AppendContent("\0", 1);
  error_ = error;
  closed_ = true;
}

boost::optional<client::ApiError> AsyncJsonStream::GetError() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return error_;
}

bool AsyncJsonStream::IsClosed() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return closed_;
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
