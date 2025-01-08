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

#include "AsyncJsonStream.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

RapidJsonByteStream::Ch RapidJsonByteStream::Peek() {
  if (ReadEmpty()) {
    SwapBuffers();
  }
  return read_buffer_[count_];
}

RapidJsonByteStream::Ch RapidJsonByteStream::Take() {
  if (ReadEmpty()) {
    SwapBuffers();
  }
  full_count_++;
  return read_buffer_[count_++];
}

size_t RapidJsonByteStream::Tell() const { return full_count_; }

// Not implemented
char* RapidJsonByteStream::PutBegin() { return 0; }
void RapidJsonByteStream::Put(char) {}
void RapidJsonByteStream::Flush() {}
size_t RapidJsonByteStream::PutEnd(char*) { return 0; }

bool RapidJsonByteStream::ReadEmpty() const {
  return count_ == read_buffer_.size();
}
bool RapidJsonByteStream::WriteEmpty() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return write_buffer_.empty();
}

void RapidJsonByteStream::AppendContent(const char* content, size_t length) {
  std::unique_lock<std::mutex> lock(mutex_);

  const auto buffer_size = write_buffer_.size();
  write_buffer_.reserve(buffer_size + length);
  write_buffer_.insert(write_buffer_.end(), content, content + length);

  cv_.notify_one();
}

void RapidJsonByteStream::SwapBuffers() {
  std::unique_lock<std::mutex> lock(mutex_);
  cv_.wait(lock, [&]() { return !write_buffer_.empty(); });
  std::swap(read_buffer_, write_buffer_);
  write_buffer_.clear();
  count_ = 0;
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
  error_ = std::move(error);
  closed_ = true;
}

boost::optional<client::ApiError> AsyncJsonStream::GetError() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return error_;
}

bool AsyncJsonStream::IsClosed() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return closed_ && (error_ || current_stream_->WriteEmpty());
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
