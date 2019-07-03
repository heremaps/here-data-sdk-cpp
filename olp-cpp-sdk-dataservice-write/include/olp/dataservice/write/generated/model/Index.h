/*
 * Copyright (C) 2019 HERE Europe B.V.
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

#include <boost/optional.hpp>

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {

typedef std::string IndexName;

/**
 @brief Type of indexes supported for index layer
 */
enum class IndexType {
  /**
   IndexValue that holds a string type with a maximum length of 40.
   */
  String,

  /**
   IndexValue that holds a int type.
   */
  Int,

  /**
   IndexValue that holds a boolean type.
   */
  Bool,

  /**
   IndexValue that holds a heretile type.
   Represents the tile id in the HERE tile map tiling scheme.
   */
  Heretile,

  /**
   IndexValue that holds a timeWindow type.
   Minimum is 10 minutes and maximum is 24 hours (1440 minutes).
   */
  TimeWindow,

  /**
   Unsupported IndexValue type
   */
  Unsupported
};

/**
 @brief IndexValue that is supported by OLP index layer
 */

class DATASERVICE_WRITE_API IndexValue {
 public:
  explicit IndexValue(IndexType type) : indexType_(type) {}
  virtual ~IndexValue() = default;
  IndexValue(const IndexValue&) = default;
  IndexValue(IndexValue&&) = default;

  IndexValue& operator=(IndexValue&&) = default;
  IndexValue& operator=(const IndexValue&) = default;
  IndexType getIndexType() const { return indexType_; }

 private:
  IndexType indexType_{IndexType::Unsupported};
};

/**
 @brief UnsupportedIndexValue that is not supported by OLP index layer
 */
class UnsupportedIndexValue final : public IndexValue {
 public:
  virtual ~UnsupportedIndexValue() = default;
  UnsupportedIndexValue(IndexType type) : IndexValue(type) {}
};

/**
 @brief BooleanIndexValue that is supported by OLP index layer indexField
 */
class BooleanIndexValue final : public IndexValue {
  bool booleanValue_{false};

 public:
  BooleanIndexValue(bool booleanValue, IndexType type) : IndexValue(type) {
    booleanValue_ = std::move(booleanValue);
  }

  const bool& GetValue() const { return booleanValue_; }
  bool& GetMutableValue() { return booleanValue_; }
  void SetValue(const bool& value) { this->booleanValue_ = value; }
};

/**
 @brief IntIndexValue that is supported by OLP index layer indexField
 */
class DATASERVICE_WRITE_API IntIndexValue final : public IndexValue {
  int64_t intValue_{0};

 public:
  IntIndexValue() = default;
  virtual ~IntIndexValue() = default;
  IntIndexValue(int64_t intValue, IndexType type) : IndexValue(type) {
    intValue_ = std::move(intValue);
  };

  const int64_t& GetValue() const { return intValue_; }
  int64_t& GetMutableValue() { return intValue_; }
  void SetValue(const int64_t& value) { this->intValue_ = value; }
};

/**
 @brief IntIndexValue that is supported by OLP index layer indexField
 */
class DATASERVICE_WRITE_API StringIndexValue final : public IndexValue {
  std::string stringValue_;

 public:
  StringIndexValue() = default;
  virtual ~StringIndexValue() = default;
  StringIndexValue(std::string stringValue, IndexType type) : IndexValue(type) {
    stringValue_ = std::move(stringValue);
  }

  std::string GetValue() const { return stringValue_; }
  std::string& GetMutableValue() { return stringValue_; }
  void SetValue(const std::string& value) { this->stringValue_ = value; }
};

/**
 @brief TimeWindowIndexValue that is supported by OLP index layer indexField
 */
class DATASERVICE_WRITE_API TimeWindowIndexValue final : public IndexValue {
  int64_t timeWindowValue_{0};

 public:
  TimeWindowIndexValue() = default;
  virtual ~TimeWindowIndexValue() = default;
  TimeWindowIndexValue(int64_t timeWindowValue, IndexType type)
      : IndexValue(type) {
    timeWindowValue_ = std::move(timeWindowValue);
  }

  const int64_t& GetValue() const { return timeWindowValue_; }
  int64_t& GetMutableValue() { return timeWindowValue_; }
  void SetValue(const int64_t& value) { this->timeWindowValue_ = value; }
};

/**
 @brief HereTileIndexValue that is supported by OLP index layer indexField
 */
class DATASERVICE_WRITE_API HereTileIndexValue final : public IndexValue {
  int64_t hereTileValue_{0};

 public:
  HereTileIndexValue() = default;
  HereTileIndexValue(int64_t hereTileValue, IndexType type) : IndexValue(type) {
    hereTileValue_ = std::move(hereTileValue);
  }

  const int64_t& GetValue() const { return hereTileValue_; }
  int64_t& GetMutableValue() { return hereTileValue_; }
  void SetValue(const int64_t& value) { this->hereTileValue_ = value; }
};

/**
 @brief EmptyIndexValue that is passed to by OLP index layer indexField
 */
class DATASERVICE_WRITE_API EmptyIndexValue final : public IndexValue {
 public:
  EmptyIndexValue(IndexType type) : IndexValue(type) {}
};

class Index {
 public:
  Index() = default;
  virtual ~Index() = default;
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
  const boost::optional<std::string>& GetCheckSum() const { return checksum_; }
  boost::optional<std::string>& GetMutableCheckSum() { return checksum_; }
  void SetCheckSum(const std::string& value) { this->checksum_ = value; }

  const boost::optional<std::map<std::string, std::string>>& GetMetadata()
      const {
    return metadata_;
  }
  boost::optional<std::map<std::string, std::string>>& GetMutableMetadata() {
    return metadata_;
  }
  void SetMetadata(const std::map<std::string, std::string>& value) {
    this->metadata_ = value;
  }

  const std::string& GetId() const { return id_; }
  std::string& GetMutableId() { return id_; }
  void SetId(const std::string& value) { this->id_ = value; }

  const std::map<IndexName, std::shared_ptr<IndexValue>>& GetIndexFields()
      const {
    return indexFields_;
  }
  std::map<IndexName, std::shared_ptr<IndexValue>>& GetMutableIndexFields() {
    return indexFields_;
  }
  void SetIndexFields(
      const std::map<IndexName, std::shared_ptr<IndexValue>>& value) {
    this->indexFields_ = value;
  }

  const boost::optional<int64_t>& GetSize() const { return size_; }
  boost::optional<int64_t>& GetMutableSize() { return size_; }
  void SetSize(const int64_t& value) { this->size_ = value; }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
