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

#pragma once

#include <atomic>
#include <functional>

#include "rapidjson/reader.h"

#include <olp/dataservice/read/model/Partitions.h>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

class PartitionsSaxHandler
    : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>,
                                          PartitionsSaxHandler> {
 public:
  using PartitionCallback = std::function<void(model::Partition)>;

  explicit PartitionsSaxHandler(PartitionCallback partition_callback);

  /// Json object events
  bool StartObject();
  bool EndObject(unsigned int);

  /// Json array events
  bool StartArray();
  bool EndArray(unsigned int);

  /// Json attributes events
  bool String(const char* str, unsigned int length, bool);
  bool Uint(unsigned int value);
  bool Default();

  /// Abort parsing
  void Abort();

 private:
  enum class State {
    kWaitForRootObject,
    kWaitForRootPartitions,
    kWaitPartitionsArray,
    kWaitForNextPartition,
    kWaitForRootObjectEnd,

    kProcessingAttribute,

    kParsingVersion,
    kParsingPartitionName,
    kParsingDataHandle,
    kParsingChecksum,
    kParsingDataSize,
    kParsingCrc,
    kParsingIgnoreAttribute,

    kParsingComplete,
  };

  State ProcessNextAttribute(const char* name, unsigned int length);

  State state_{State::kWaitForRootObject};
  model::Partition partition_;
  PartitionCallback partition_callback_;
  std::atomic_bool continue_parsing_{true};
};

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
