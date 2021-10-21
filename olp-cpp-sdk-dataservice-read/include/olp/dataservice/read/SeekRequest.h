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

#pragma once

#include <vector>

#include <olp/dataservice/read/model/StreamOffsets.h>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Requests to seek an offset for a stream layer.
 */
class DATASERVICE_READ_API SeekRequest final {
 public:
  /// An alias for the stream offsets.
  using StreamOffsets = model::StreamOffsets;

  /**
   * @brief Sets offsets for the request.
   *
   * No offset by default.
   *
   * @param offsets The stream offsets.
   *
   * @return A reference to the updated `SeekRequest` instance.
   */
  SeekRequest& WithOffsets(StreamOffsets offsets) {
    stream_offsets_ = offsets;
    return *this;
  }

  /**
   * @brief Gets the stream offsets of the request.
   *
   * @return The current stream offsets.
   */
  const model::StreamOffsets& GetOffsets() const { return stream_offsets_; }

 private:
  StreamOffsets stream_offsets_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
