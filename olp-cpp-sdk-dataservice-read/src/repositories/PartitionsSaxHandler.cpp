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

#include "PartitionsSaxHandler.h"

#include <olp/core/utils/WarningWorkarounds.h>

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

bool PartitionsSaxHandler::on_document_begin(boost::json::error_code&) {
  return continue_parsing_;
}

bool PartitionsSaxHandler::on_document_end(boost::json::error_code&) {
  return continue_parsing_;
}

bool PartitionsSaxHandler::on_array_begin(boost::json::error_code&) {
  // We expect only a single array in the whole response
  if (state_ != State::kWaitPartitionsArray) {
    return false;
  }

  state_ = State::kWaitForNextPartition;

  return continue_parsing_;
}

bool PartitionsSaxHandler::on_array_end(std::size_t, boost::json::error_code&) {
  key_.clear();
  value_.clear();

  if (state_ != State::kWaitForNextPartition) {
    return false;
  }

  state_ = State::kWaitForRootObjectEnd;
  return continue_parsing_;
}

bool PartitionsSaxHandler::on_object_begin(boost::json::error_code&) {
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

bool PartitionsSaxHandler::on_object_end(std::size_t,
                                         boost::json::error_code&) {
  key_.clear();
  value_.clear();

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

bool PartitionsSaxHandler::on_string_part(boost::json::string_view s,
                                          std::size_t,
                                          boost::json::error_code&) {
  value_ += s;
  return continue_parsing_;
}

bool PartitionsSaxHandler::on_string(boost::json::string_view s, std::size_t,
                                     boost::json::error_code&) {
  value_ += s;

  switch (state_) {
    case State::kParsingPartitionName:
      partition_.SetPartition(value_);
      break;
    case State::kParsingDataHandle:
      partition_.SetDataHandle(value_);
      break;
    case State::kParsingChecksum:
      partition_.SetChecksum(value_);
      break;
    case State::kParsingCrc:
      partition_.SetCrc(value_);
      break;
    case State::kParsingIgnoreAttribute:
      break;

    case State::kProcessingAttribute:        // Not expected
    case State::kWaitForRootPartitions:      // Not expected
    case State::kWaitForRootObject:          // Not expected
    case State::kWaitPartitionsArray:        // Not expected
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

  value_.clear();

  return continue_parsing_;
}

bool PartitionsSaxHandler::on_key_part(boost::json::string_view s, std::size_t,
                                       boost::json::error_code&) {
  key_ += s;
  return continue_parsing_;
}

bool PartitionsSaxHandler::on_key(boost::json::string_view s, std::size_t,
                                  boost::json::error_code&) {
  key_ += s;

  switch (state_) {
    case State::kProcessingAttribute:
      state_ = ProcessNextAttribute(key_.data(), key_.size());
      break;

    case State::kWaitForRootPartitions:
      if (HashStringToInt("partitions") != HashStringToInt(key_.data())) {
        return false;
      }
      state_ = State::kWaitPartitionsArray;
      break;

    case State::kParsingPartitionName:       // Not expected
    case State::kParsingDataHandle:          // Not expected
    case State::kParsingChecksum:            // Not expected
    case State::kParsingCrc:                 // Not expected
    case State::kParsingIgnoreAttribute:     // Not expected
    case State::kWaitForRootObject:          // Not expected
    case State::kWaitForNextPartition:       // Not expected
    case State::kWaitForRootObjectEnd:       // Not expected
    case State::kParsingVersion:             // Not expected
    case State::kParsingDataSize:            // Not expected
    case State::kParsingCompressedDataSize:  // Not expected
    default:
      return false;
  };

  key_.clear();

  return continue_parsing_;
}

bool PartitionsSaxHandler::on_number_part(boost::json::string_view,
                                          boost::json::error_code&) {
  return continue_parsing_;
}

bool PartitionsSaxHandler::on_int64(int64_t i, boost::json::string_view,
                                    boost::json::error_code&) {
  if (state_ == State::kParsingVersion) {
    partition_.SetVersion(i);
  } else if (state_ == State::kParsingDataSize) {
    partition_.SetDataSize(i);
  } else if (state_ == State::kParsingCompressedDataSize) {
    partition_.SetCompressedDataSize(i);
  } else {
    return false;
  }

  state_ = State::kProcessingAttribute;
  return continue_parsing_;
}

bool PartitionsSaxHandler::on_uint64(uint64_t u, boost::json::string_view,
                                     boost::json::error_code&) {
  if (state_ == State::kParsingVersion) {
    partition_.SetVersion(u);
  } else if (state_ == State::kParsingDataSize) {
    partition_.SetDataSize(u);
  } else if (state_ == State::kParsingCompressedDataSize) {
    partition_.SetCompressedDataSize(u);
  } else {
    return false;
  }

  state_ = State::kProcessingAttribute;
  return continue_parsing_;
}

bool PartitionsSaxHandler::on_double(double, boost::json::string_view,
                                     boost::json::error_code&) {
  return continue_parsing_;
}

bool PartitionsSaxHandler::on_bool(bool, boost::json::error_code&) {
  return continue_parsing_;
}

bool PartitionsSaxHandler::on_null(boost::json::error_code&) {
  return continue_parsing_;
}

bool PartitionsSaxHandler::on_comment_part(boost::json::string_view,
                                           boost::json::error_code&) {
  return continue_parsing_;
}

bool PartitionsSaxHandler::on_comment(boost::json::string_view,
                                      boost::json::error_code&) {
  return continue_parsing_;
}

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
