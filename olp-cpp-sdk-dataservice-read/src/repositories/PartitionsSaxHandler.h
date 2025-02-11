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

#pragma once

#include <atomic>
#include <functional>

#include <boost/json/value.hpp>

#include <olp/dataservice/read/model/Partitions.h>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

class PartitionsSaxHandler {
 public:
  using PartitionCallback = std::function<void(model::Partition)>;

  explicit PartitionsSaxHandler(PartitionCallback partition_callback);

  static constexpr std::size_t max_object_size =
      boost::json::object::max_size();

  static constexpr std::size_t max_array_size = boost::json::array::max_size();

  static constexpr std::size_t max_key_size = boost::json::string::max_size();

  static constexpr std::size_t max_string_size =
      boost::json::string::max_size();

  /// Called once when the JSON parsing begins.
  ///
  /// @return `true` on success.
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_document_begin(boost::json::error_code& ec);

  /// Called when the JSON parsing is done.
  ///
  /// @return `true` on success.
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_document_end(boost::json::error_code& ec);

  /// Called when the beginning of an array is encountered.
  ///
  /// @return `true` on success.
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_array_begin(boost::json::error_code& ec);

  /// Called when the end of the current array is encountered.
  ///
  /// @return `true` on success.
  /// @param n The number of elements in the array.
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_array_end(std::size_t n, boost::json::error_code& ec);

  /// Called when the beginning of an object is encountered.
  ///
  /// @return `true` on success.
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_object_begin(boost::json::error_code& ec);

  /// Called when the end of the current object is encountered.
  ///
  /// @return `true` on success.
  /// @param n The number of elements in the object.
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_object_end(std::size_t n, boost::json::error_code& ec);

  /// Called with characters corresponding to part of the current string.
  ///
  /// @return `true` on success.
  /// @param s The partial characters
  /// @param n The total size of the string thus far
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_string_part(boost::json::string_view s, std::size_t n,
                      boost::json::error_code& ec);

  /// Called with the last characters corresponding to the current string.
  ///
  /// @return `true` on success.
  /// @param s The remaining characters
  /// @param n The total size of the string
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_string(boost::json::string_view s, std::size_t n,
                 boost::json::error_code& ec);

  /// Called with characters corresponding to part of the current key.
  ///
  /// @return `true` on success.
  /// @param s The partial characters
  /// @param n The total size of the key thus far
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_key_part(boost::json::string_view s, std::size_t n,
                   boost::json::error_code& ec);

  /// Called with the last characters corresponding to the current key.
  ///
  /// @return `true` on success.
  /// @param s The remaining characters
  /// @param n The total size of the key
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_key(boost::json::string_view s, std::size_t n,
              boost::json::error_code& ec);

  /// Called with the characters corresponding to part of the current number.
  ///
  /// @return `true` on success.
  /// @param s The partial characters
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_number_part(boost::json::string_view s, boost::json::error_code& ec);

  /// Called when a signed integer is parsed.
  ///
  /// @return `true` on success.
  /// @param i The value
  /// @param s The remaining characters
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_int64(int64_t i, boost::json::string_view s,
                boost::json::error_code& ec);

  /// Called when an unsigend integer is parsed.
  ///
  /// @return `true` on success.
  /// @param u The value
  /// @param s The remaining characters
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_uint64(uint64_t u, boost::json::string_view s,
                 boost::json::error_code& ec);

  /// Called when a double is parsed.
  ///
  /// @return `true` on success.
  /// @param d The value
  /// @param s The remaining characters
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_double(double d, boost::json::string_view s,
                 boost::json::error_code& ec);

  /// Called when a boolean is parsed.
  ///
  /// @return `true` on success.
  /// @param b The value
  /// @param s The remaining characters
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_bool(bool b, boost::json::error_code& ec);

  /// Called when a null is parsed.
  ///
  /// @return `true` on success.
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_null(boost::json::error_code& ec);

  /// Called with characters corresponding to part of the current comment.
  ///
  /// @return `true` on success.
  /// @param s The partial characters.
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_comment_part(boost::json::string_view s, boost::json::error_code& ec);

  /// Called with the last characters corresponding to the current comment.
  ///
  /// @return `true` on success.
  /// @param s The remaining characters
  /// @param ec Set to the error, if any occurred.
  ///
  bool on_comment(boost::json::string_view s, boost::json::error_code& ec);

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
    kParsingCompressedDataSize,
    kParsingCrc,
    kParsingIgnoreAttribute,

    kParsingComplete,
  };

  static State ProcessNextAttribute(const char* name, unsigned int length);

  State state_{State::kWaitForRootObject};
  std::string key_;
  std::string value_;
  model::Partition partition_;
  PartitionCallback partition_callback_;
  std::atomic_bool continue_parsing_{true};
};

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
