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

#include <olp/dataservice/read/DataServiceReadApi.h>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/**
 * @brief An offset in a specific partition of a stream layer.
 */
class DATASERVICE_READ_API StreamOffset final {
 public:
  StreamOffset() = default;

  /**
   * @brief Gets the partition of the stream layer for which this offset
   * applies.
   *
   * @return The partition of the stream layer for which this offset applies.
   */
  int32_t GetPartition() const { return partition_; }
  /**
   * @brief Sets the partition of the stream layer.
   *
   * @param value The partition of the stream layer for which this offset
   * applies.
   */
  void SetPartition(int32_t value) { partition_ = value; }

  /**
   * @brief Gets the offset in the partition of the stream layer.
   *
   * @return The offset in the stream layer partition.
   */
  int64_t GetOffset() const { return offset_; };
  /**
   * @brief Sets the offset for the request.
   *
   * @param value The offset in the stream layer partition.
   */
  void SetOffset(int64_t value) { offset_ = value; }

 private:
  /// The partition of the stream layer for which this offset applies. It is not
  /// the same as [Partitions
  /// object](https://developer.here.com/olp/documentation/data-api/data_dev_guide/rest/layers/partitions.html).
  int32_t partition_;

  /// The offset in the partition of the stream layer.
  int64_t offset_;
};

/**
 * @brief Represents a list of offsets.
 */
class DATASERVICE_READ_API StreamOffsets final {
 public:
  StreamOffsets() = default;

  /**
   * @brief Gets the list of offsets.
   *
   * @return The list of offsets.
   */
  const std::vector<StreamOffset>& GetOffsets() const { return offsets_; };
  /**
   * @brief Sets the list of offsets.
   *
   * @param value The list of offsets.
   */
  void SetOffsets(std::vector<StreamOffset> value) {
    offsets_ = std::move(value);
  };

 private:
  std::vector<StreamOffset> offsets_;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
