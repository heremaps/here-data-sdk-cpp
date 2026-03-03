/*
 * Copyright (C) 2023-2026 HERE Europe B.V.
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

#include "PartitionsSaxHandler.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

namespace {

constexpr unsigned long long int HashStringToInt(
    const char* str, unsigned long long int hash = 0) {
  return (*str == 0) ? hash : 101 * HashStringToInt(str + 1) + *str;
}
}  // namespace

PartitionsSaxHandler::PartitionsSaxHandler(PartitionCallback partition_callback)
    : partition_callback_(std::move(partition_callback)) {}

bool PartitionsSaxHandler::on_object_begin(boost::json::error_code& ec) {
  if (state_ == State::kWaitForRootObject) {
    state_ = State::kWaitForRootPartitions;
    return CanContinue(ec);
  }

  if (state_ != State::kWaitForNextPartition) {
    return NotSupported(ec);
  }

  state_ = State::kProcessingAttribute;

  return CanContinue(ec);
}
bool PartitionsSaxHandler::String(const std::string& str, error_code& ec) {
  switch (state_) {
    case State::kProcessingAttribute:
      state_ = ProcessNextAttribute(str);
      return CanContinue(ec);

    case State::kWaitForRootPartitions: {
      if (HashStringToInt("partitions") == HashStringToInt(str.c_str())) {
        state_ = State::kWaitPartitionsArray;
        return CanContinue(ec);
      }
      return NotSupported(ec);
    }

    case State::kParsingPartitionName:
      partition_.SetPartition(str);
      break;
    case State::kParsingDataHandle:
      partition_.SetDataHandle(str);
      break;
    case State::kParsingChecksum:
      partition_.SetChecksum(str);
      break;
    case State::kParsingCrc:
      partition_.SetCrc(str);
      break;
    case State::kParsingIgnoreAttribute:
      break;

    case State::kWaitForRootObject:          // Not expected
    case State::kWaitForNextPartition:       // Not expected
    case State::kWaitForRootObjectEnd:       // Not expected
    case State::kParsingVersion:             // Version is not a string
    case State::kParsingDataSize:            // DataSize is not a string
    case State::kParsingCompressedDataSize:  // CompressedDataSize is not a
                                             // string
    default:
      return false;
  }

  state_ = State::kProcessingAttribute;

  return CanContinue(ec);
}

bool PartitionsSaxHandler::on_int64(const int64_t value, string_view,
                                    error_code& ec) {
  if (state_ == State::kParsingVersion) {
    partition_.SetVersion(value);
  } else if (state_ == State::kParsingDataSize) {
    partition_.SetDataSize(value);
  } else if (state_ == State::kParsingCompressedDataSize) {
    partition_.SetCompressedDataSize(value);
  } else {
    return NotSupported(ec);
  }

  state_ = State::kProcessingAttribute;
  return CanContinue(ec);
}

bool PartitionsSaxHandler::on_object_end(std::size_t, error_code& ec) {
  if (state_ == State::kWaitForRootObjectEnd) {
    state_ = State::kParsingComplete;
    return CanContinue(ec);  // complete
  }

  if (state_ != State::kProcessingAttribute) {
    return NotSupported(ec);
  }

  if (partition_.GetDataHandle().empty() || partition_.GetPartition().empty()) {
    return NotSupported(ec);  // partition is not valid
  }

  partition_callback_(std::move(partition_));

  state_ = State::kWaitForNextPartition;

  return CanContinue(ec);
}

bool PartitionsSaxHandler::on_array_begin(boost::json::error_code& ec) {
  // We expect only a single array in whol response
  if (state_ != State::kWaitPartitionsArray) {
    return NotSupported(ec);
  }

  state_ = State::kWaitForNextPartition;

  return CanContinue(ec);
}

bool PartitionsSaxHandler::on_array_end(std::size_t,
                                        boost::json::error_code& ec) {
  if (state_ != State::kWaitForNextPartition) {
    return NotSupported(ec);
  }

  state_ = State::kWaitForRootObjectEnd;
  return CanContinue(ec);
}

void PartitionsSaxHandler::Abort() { continue_parsing_.store(false); }

PartitionsSaxHandler::State PartitionsSaxHandler::ProcessNextAttribute(
    const std::string& name) {
  switch (HashStringToInt(name.c_str())) {
    case HashStringToInt("dataHandle"):
      return State::kParsingDataHandle;
    case HashStringToInt("partition"):
      return State::kParsingPartitionName;
    case HashStringToInt("checksum"):
      return State::kParsingChecksum;
    case HashStringToInt("dataSize"):
      return State::kParsingDataSize;
    case HashStringToInt("compressedDataSize"):
      return State::kParsingCompressedDataSize;
    case HashStringToInt("version"):
      return State::kParsingVersion;
    case HashStringToInt("crc"):
      return State::kParsingCrc;
    default:
      return State::kParsingIgnoreAttribute;
  }
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
