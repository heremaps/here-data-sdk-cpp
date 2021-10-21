/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include <map>
#include <memory>
#include <string>
#include <utility>

#include <boost/optional.hpp>

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {

typedef std::string IndexName;

/// Types of index supported by an index layer.
enum class DATASERVICE_WRITE_API IndexType {
  /// The index value of the string type. The maximum length is 40 characters.
  String,

  /// The index value of the integer type.
  Int,

  /// The index value of the boolean type.
  Bool,

  /**
   * @brief The index value of the HERE tile type.
   *
   * Represents the tile ID in the HERE map tiling scheme. 
   */
  Heretile,


  /**
   * @brief The index value of the time window type.
   *
   * The minimum value is 10 minutes, and the maximum is 24 hours (1440 minutes).
   */
  TimeWindow,

  /// Values that are not supported by the index layer.
  Unsupported
};

/// Represents values supported by the HERE platform index layer.
class DATASERVICE_WRITE_API IndexValue {
 public:
  /**
   * @brief Creates the `IndexValue` instance.
   *
   * @param type The`IndexType` instance.
   */
  explicit IndexValue(IndexType type) : indexType_(type) {}
  virtual ~IndexValue() = default;

  /// A default copy constructor.
  IndexValue(const IndexValue&) = default;

  /// A default move constructor.
  IndexValue(IndexValue&&) = default;

  /// A default move assignment operator.
  IndexValue& operator=(IndexValue&&) = default;

  /// A default copy assignment operator.
  IndexValue& operator=(const IndexValue&) = default;

  /**
   * @brief Gets the index value type.
   *
   * @return The index value type.
   */
  IndexType getIndexType() const { return indexType_; }

 private:
  IndexType indexType_{IndexType::Unsupported};
};

/// Represents values that are not supported by the index layer.
class DATASERVICE_WRITE_API UnsupportedIndexValue final : public IndexValue {
 public:
 /**
   * @brief Creates the `UnsupportedIndexValue` instance.
   *
   * @param type The`IndexType` instance.
   */
  explicit UnsupportedIndexValue(IndexType type) : IndexValue(type) {}
};

/// Represents the index layer of the boolean type.
class DATASERVICE_WRITE_API BooleanIndexValue final : public IndexValue {
  bool booleanValue_{false};

 public:
  /**
   * @brief Creates the `BooleanIndexValue` instance.
   *
   * @param booleanValue The boolean value.
   * @param type The `IndexType` instance.
   */
  BooleanIndexValue(bool booleanValue, IndexType type) : IndexValue(type) {
    booleanValue_ = std::move(booleanValue);
  }

  /**
   * @brief Gets the boolean value of the index layer.
   *
   * @return The boolean value.
   */
  const bool& GetValue() const { return booleanValue_; }

  /**
   * @brief Gets a mutable reference to the boolean value of the index layer.
   *
   * @return The mutable reference to the boolean value.
   */
  bool& GetMutableValue() { return booleanValue_; }

  /**
   * @brief Sets the boolean value.
   *
   * @param value The boolean value.
   */
  void SetValue(const bool& value) { this->booleanValue_ = value; }
};

/// Represents the index layer of the integer type.
class DATASERVICE_WRITE_API IntIndexValue final : public IndexValue {
  int64_t intValue_{0};

 public:
  virtual ~IntIndexValue() = default;

  /**
   * @brief Creates the `IntIndexValue` instance.
   *
   * @param intValue The integer value.
   * @param type The `IndexType` instance.
   */
  IntIndexValue(int64_t intValue, IndexType type) : IndexValue(type) {
    intValue_ = std::move(intValue);
  }

  /**
   * @brief Gets the integer value of the index layer.
   *
   * @return The integer value.
   */
  const int64_t& GetValue() const { return intValue_; }

  /**
   * @brief Gets a mutable reference to the integer value of the index layer.
   *
   * @return The mutable reference to the integer value.
   */
  int64_t& GetMutableValue() { return intValue_; }

  /**
   * @brief Sets the integer value.
   *
   * @param value The integer value.
   */
  void SetValue(const int64_t& value) { this->intValue_ = value; }
};

/// Represents the index layer of the string type.
class DATASERVICE_WRITE_API StringIndexValue final : public IndexValue {
  std::string stringValue_;

 public:
  virtual ~StringIndexValue() = default;

  /**
   * @brief Creates the `StringIndexValue` instance.
   *
   * @param stringValue The string value.
   * @param type The `IndexType` instance.
   */
  StringIndexValue(std::string stringValue, IndexType type) : IndexValue(type) {
    stringValue_ = std::move(stringValue);
  }

  /**
   * @brief Gets the string value of the index layer.
   *
   * @return The string value.
   */
  std::string GetValue() const { return stringValue_; }

  /**
   * @brief Gets a mutable reference to the string value of the index layer.
   *
   * @return The string value.
   */
  std::string& GetMutableValue() { return stringValue_; }

  /**
   * @brief Sets the string value.
   *
   * @param value The string value.
   */
  void SetValue(const std::string& value) { this->stringValue_ = value; }
};

/// Represents the index layer of the time window type.
class DATASERVICE_WRITE_API TimeWindowIndexValue final : public IndexValue {
  int64_t timeWindowValue_{0};

 public:
  virtual ~TimeWindowIndexValue() = default;

  /**
   * @brief Creates the `TimeWindowIndexValue` instance.
   *
   * @param timeWindowValue The time window value.
   * @param type  The `IndexType` instance.
   */
  TimeWindowIndexValue(int64_t timeWindowValue, IndexType type)
      : IndexValue(type) {
    timeWindowValue_ = std::move(timeWindowValue);
  }

  /**
   * @brief Gets the time vindow value of the index layer.
   *
   * @return The time window value.
   */
  const int64_t& GetValue() const { return timeWindowValue_; }

  /**
   * @brief Gets a mutable reference to the time window value of the index layer.
   *
   * @return The mutable reference to the time window value.
   */
  int64_t& GetMutableValue() { return timeWindowValue_; }

  /**
   * @brief Sets the time window value.
   *
   * @param value The time window value.
   */
  void SetValue(const int64_t& value) { this->timeWindowValue_ = value; }
};

/// Represents the index layer of the HERE tile type.
class DATASERVICE_WRITE_API HereTileIndexValue final : public IndexValue {
  int64_t hereTileValue_{0};

 public:
  /**
   * @brief Creates the `HereTileIndexValue` instance.
   *
   * @param hereTileValue The HERE tile value.
   * @param type The `IndexType` instance.
   */
  HereTileIndexValue(int64_t hereTileValue, IndexType type) : IndexValue(type) {
    hereTileValue_ = std::move(hereTileValue);
  }

  /**
   * @brief Gets the HERE tile value of the index layer.
   *
   * @return The HERE tile value.
   */
  const int64_t& GetValue() const { return hereTileValue_; }

  /**
   * @brief Gets a mutable reference to the HERE tile value
   * of the index layer.
   *
   * @return The mutable reference to the HERE tile value.
   */
  int64_t& GetMutableValue() { return hereTileValue_; }

  /**
   * @brief Sets the HERE tile value.
   *
   * @param value The HERE tile value.
   */
  void SetValue(const int64_t& value) { this->hereTileValue_ = value; }
};

/// Represents the index layer with an empty index value.
class DATASERVICE_WRITE_API EmptyIndexValue final : public IndexValue {
 public:
  /**
   * @brief Creates the `EmptyIndexValue` instance.
   *
   * @param type The `IndexType` instance.
   */
  explicit EmptyIndexValue(IndexType type) : IndexValue(type) {}
};

/// Represents the index layer.
class DATASERVICE_WRITE_API Index final {
 public:
  /// A default constructor.
  Index() = default;
  virtual ~Index() = default;

  /**
   * @brief Creates the `Index` insatnce.
   *
   * @param uuid The unique ID.
   * @param indexFields The index value type.
   */
  Index(std::string uuid,
        std::map<IndexName, std::shared_ptr<IndexValue>> indexFields)
      : id_(uuid), indexFields_(indexFields) {}

 private:
  boost::optional<std::string> checksum_;
  boost::optional<std::map<std::string, std::string>> metadata_;
  boost::optional<int64_t> size_;
  std::string id_;
  std::map<IndexName, std::shared_ptr<IndexValue>> indexFields_;

 public:
  /**
   * @brief Gets the checksum of the index layer.
   *
   * @return The checksum.
   */
  const boost::optional<std::string>& GetCheckSum() const { return checksum_; }

  /**
   * @brief Gets a mutable reference to the checksum of the index layer.
   *
   * @return The mutable reference to the checksum.
   */
  boost::optional<std::string>& GetMutableCheckSum() { return checksum_; }

  /**
   * @brief Sets the checksum.
   *
   * @param value The checksum.
   */
  void SetCheckSum(const std::string& value) { this->checksum_ = value; }

  /**
   * @brief Gets the metadata of the index layer.
   *
   * @return The metadata of the index layer.
   */
  const boost::optional<std::map<std::string, std::string>>& GetMetadata()
      const {
    return metadata_;
  }

  /**
   * @brief Gets a mutable reference to the metadata of the index layer.
   *
   * @return The mutable reference to the metadata.
   */
  boost::optional<std::map<std::string, std::string>>& GetMutableMetadata() {
    return metadata_;
  }

  /**
   * @brief Sets the metadata of the index layer.
   *
   * @param value The metadata of the index layer.
   */
  void SetMetadata(const std::map<std::string, std::string>& value) {
    this->metadata_ = value;
  }

  /**
   * @brief Gets the index layer ID.
   *
   * @return The index layer ID.
   */
  const std::string& GetId() const { return id_; }

  /**
   * @brief Gets a mutable reference to the index layer ID.
   *
   * @return The mutable reference to the index layer ID.
   */
  std::string& GetMutableId() { return id_; }

  /**
   * @brief Sets the index layer ID.
   *
   * @param value The index layer ID.
   */
  void SetId(const std::string& value) { this->id_ = value; }

  /**
   * @brief Gets the index value type.
   *
   * @return The index value type.
   */
  const std::map<IndexName, std::shared_ptr<IndexValue>>& GetIndexFields()
      const {
    return indexFields_;
  }

  /**
   * @brief Gets a mutable reference to the index value type.
   *
   * @return The mutable reference to the index value type.
   */
  std::map<IndexName, std::shared_ptr<IndexValue>>& GetMutableIndexFields() {
    return indexFields_;
  }

  /**
   * @brief Sets the index value type.
   *
   * @param value The index vale type.
   */
  void SetIndexFields(
      const std::map<IndexName, std::shared_ptr<IndexValue>>& value) {
    this->indexFields_ = value;
  }

  /**
   * @brief Gets the size of the index layer.
   *
   * @return The size of the index layer.
   */
  const boost::optional<int64_t>& GetSize() const { return size_; }

  /**
   * @brief Gets a mutable reference to the size of the index layer.
   *
   * @return The mutable reference to the size of the index layer.
   */
  boost::optional<int64_t>& GetMutableSize() { return size_; }

  /**
   * @brief Sets the size of the index layer.
   *
   * @param value The size of the index layer.
   */
  void SetSize(const int64_t& value) { this->size_ = value; }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
