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

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/**
 * @brief An offset in a specific partition of the stream layer.
 */
class StreamOffset {
 public:
  StreamOffset() = default;
  virtual ~StreamOffset() = default;

  int32_t GetPartition() const { return partition_; }
  void SetPartition(int32_t value) { partition_ = value; }

  int64_t GetOffset() const { return offset_; };
  void SetOffset(int64_t value) { offset_ = value; }

 private:
  /**
   * @brief The partition of the stream layer for which this offset applies. It
   * is not the same as [Partitions
   * object](https://developer.here.com/olp/documentation/data-user-guide/shared_content/topics/olp/concepts/partitions.html).
   */
  int32_t partition_;

  /**
   * @brief The offset in the partition of the stream layer.
   */
  int64_t offset_;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
