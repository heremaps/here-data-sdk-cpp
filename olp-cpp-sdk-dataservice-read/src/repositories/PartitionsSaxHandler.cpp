/*
 * Copyright (C) 2023-2024 HERE Europe B.V.
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

bool PartitionsSaxHandler::StartObject() {
  if (state_ == State::kWaitForRootObject) {
    state_ = State::kWaitForRootPartitions;
    return continue_parsing_;
  }

  if (state_ != State::kWaitForNextPartition) {
    return false;
  }

  state_ = State::kProcessingAttribute;

  return continue_parsing_;
}

bool PartitionsSaxHandler::String(const char* str, unsigned int length, bool) {
  switch (state_) {
    case State::kProcessingAttribute:
      state_ = ProcessNextAttribute(str, length);
      return continue_parsing_;

    case State::kWaitForRootPartitions:
      if (HashStringToInt("partitions") == HashStringToInt(str)) {
        state_ = State::kWaitPartitionsArray;
        return continue_parsing_;
      } else {
        return false;
      }

    case State::kParsingPartitionName:
      partition_.SetPartition(std::string(str, length));
      break;
    case State::kParsingDataHandle:
      partition_.SetDataHandle(std::string(str, length));
      break;
    case State::kParsingChecksum:
      partition_.SetChecksum(std::string(str, length));
      break;
    case State::kParsingCrc:
      partition_.SetCrc(std::string(str, length));
      break;
    case State::kParsingIgnoreAttribute:
      break;

    case State::kWaitForRootObject:     // Not expected
    case State::kWaitForNextPartition:  // Not expected
    case State::kWaitForRootObjectEnd:  // Not expected
    case State::kParsingVersion:        // Version is not a string
    case State::kParsingDataSize:       // DataSize is not a string
    default:
      return false;
  }

  state_ = State::kProcessingAttribute;

  return continue_parsing_;
}

bool PartitionsSaxHandler::Uint(unsigned int value) {
  if (state_ == State::kParsingVersion) {
    partition_.SetVersion(value);
  } else if (state_ == State::kParsingDataSize) {
    partition_.SetDataSize(value);
  } else {
    return false;
  }

  state_ = State::kProcessingAttribute;
  return continue_parsing_;
}

bool PartitionsSaxHandler::EndObject(unsigned int) {
  if (state_ == State::kWaitForRootObjectEnd) {
    state_ = State::kParsingComplete;
    return true;  // complete
  }

  if (state_ != State::kProcessingAttribute) {
    return false;
  }

  if (partition_.GetDataHandle().empty() || partition_.GetPartition().empty()) {
    return false;  // partition is not valid
  }

  partition_callback_(std::move(partition_));

  state_ = State::kWaitForNextPartition;

  return continue_parsing_;
}

bool PartitionsSaxHandler::StartArray() {
  // We expect only a single array in whol response
  if (state_ != State::kWaitPartitionsArray) {
    return false;
  }

  state_ = State::kWaitForNextPartition;

  return continue_parsing_;
}

bool PartitionsSaxHandler::EndArray(unsigned int) {
  if (state_ != State::kWaitForNextPartition) {
    return false;
  }

  state_ = State::kWaitForRootObjectEnd;
  return continue_parsing_;
}

bool PartitionsSaxHandler::Default() { return false; }

void PartitionsSaxHandler::Abort() { continue_parsing_.store(false); }

PartitionsSaxHandler::State PartitionsSaxHandler::ProcessNextAttribute(
    const char* name, unsigned int /*length*/) {
  switch (HashStringToInt(name)) {
    case HashStringToInt("dataHandle"):
      return State::kParsingDataHandle;
    case HashStringToInt("partition"):
      return State::kParsingPartitionName;
    case HashStringToInt("checksum"):
      return State::kParsingChecksum;
    case HashStringToInt("dataSize"):
      return State::kParsingDataSize;
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
