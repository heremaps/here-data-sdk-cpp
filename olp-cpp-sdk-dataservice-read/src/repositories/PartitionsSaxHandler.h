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

#pragma once

#include <atomic>
#include <functional>

#include <boost/json/error.hpp>
#include <boost/json/string_view.hpp>
#include <boost/json/system_error.hpp>

#include <olp/dataservice/read/model/Partitions.h>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

class PartitionsSaxHandler {
 public:
  using PartitionCallback = std::function<void(model::Partition)>;
  using string_view = boost::json::string_view;
  using error_code = boost::json::error_code;

  explicit PartitionsSaxHandler(PartitionCallback partition_callback);

  /// Abort parsing
  void Abort();

  // boost::json::basic_parser handler object methods
  static constexpr std::size_t max_array_size = -1;
  static constexpr std::size_t max_object_size = -1;
  static constexpr std::size_t max_string_size = -1;
  static constexpr std::size_t max_key_size = -1;

  bool on_document_begin(error_code&) { return continue_parsing_; }
  bool on_document_end(error_code&) { return continue_parsing_; }

  bool on_array_begin(error_code& ec);
  bool on_array_end(std::size_t size, error_code& ec);

  bool on_object_begin(error_code& ec);
  bool on_object_end(std::size_t size, error_code& ec);

  bool on_string_part(string_view str, std::size_t size, error_code& ec) {
    str.length() == size ? value_ = str : value_ += str;
    return CanContinue(ec);
  }

  bool on_string(string_view str, std::size_t size, error_code& ec) {
    str.length() == size ? value_ = str : value_ += str;
    return String(value_, ec);
  }

  bool on_key_part(string_view str, std::size_t size, error_code& ec) {
    str.length() == size ? key_ = str : key_ += str;
    return CanContinue(ec);
  }

  bool on_key(string_view str, std::size_t size, error_code& ec) {
    str.length() == size ? key_ = str : key_ += str;
    return String(key_, ec);
  }

  bool on_number_part(string_view, error_code& ec) const {
    return CanContinue(ec);
  }

  bool on_uint64(uint64_t value, string_view string, error_code& ec) {
    return on_int64(static_cast<int64_t>(value), string, ec);
  }

  bool on_int64(int64_t value, string_view, error_code& ec);

  static bool on_double(double, string_view, error_code& ec) {
    return NotSupported(ec);
  }

  static bool on_bool(bool, error_code& ec) { return NotSupported(ec); }
  static bool on_null(error_code& ec) { return NotSupported(ec); }

  bool on_comment_part(string_view, error_code& ec) const {
    return CanContinue(ec);
  }

  bool on_comment(string_view, error_code& ec) const { return CanContinue(ec); }

 private:
  bool String(const std::string& str, error_code& ec);

  bool CanContinue(error_code& ec) const {
    return continue_parsing_ || NotSupported(ec);
  }

  static bool NotSupported(error_code& ec) {
    ec = boost::json::error::extra_data;
    return false;
  }
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
    kParsingCompressedDataSize,
    kParsingCrc,
    kParsingIgnoreAttribute,

    kParsingComplete,
  };

  static State ProcessNextAttribute(const std::string& name);

  State state_{State::kWaitForRootObject};
  model::Partition partition_;
  PartitionCallback partition_callback_;
  std::string key_;
  std::string value_;
  std::atomic_bool continue_parsing_{true};
};

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
