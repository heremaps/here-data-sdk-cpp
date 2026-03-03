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

#include <gtest/gtest.h>

#include "repositories/PartitionsSaxHandler.h"

namespace {

namespace repository = olp::dataservice::read::repository;
namespace model = olp::dataservice::read::model;

const char* kPartitions = "partitions";

const char* kDataHandle = "dataHandle";
const char* kPartition = "partition";
const char* kChecksum = "checksum";
const char* kDataSize = "dataSize";
const char* kCompressedDataSize = "compressedDataSize";
const char* kVersion = "version";
const char* kCrc = "crc";

const char* kDataHandleValue = "DEADBEEF";
const char* kPartitionValue = "123456";
const char* kChecksumValue = "0123456789abcdef";
const char* kCrcValue = "abcdef";

unsigned int len(const char* str) {
  return static_cast<unsigned int>(strlen(str));
}

TEST(PartitionsSaxHandlerTest, NormalFlow) {
  model::Partition parsed_partition;
  auto callback = [&](const model::Partition& partition) {
    parsed_partition = partition;
  };

  repository::PartitionsSaxHandler handler(callback);

  boost::json::error_code error_code;
  ASSERT_TRUE(handler.on_object_begin(error_code));
  ASSERT_TRUE(handler.on_key(kPartitions, len(kPartitions), error_code));
  ASSERT_TRUE(handler.on_array_begin(error_code));
  ASSERT_TRUE(handler.on_object_begin(error_code));

  boost::json::string_view key_part_1{kDataHandle, 2};
  boost::json::string_view key_part_2{kDataHandle + key_part_1.size(), 3};
  boost::json::string_view key_part_last{
      kDataHandle + key_part_1.size() + key_part_2.size(),
      len(kDataHandle) - key_part_1.size() - key_part_2.size()};
  ASSERT_TRUE(handler.on_key_part(key_part_1, key_part_1.size(), error_code));
  ASSERT_TRUE(handler.on_key_part(
      key_part_2, key_part_1.size() + key_part_2.size(), error_code));
  ASSERT_TRUE(handler.on_key(key_part_last, len(kDataHandle), error_code));

  boost::json::string_view value_part_1{kDataHandleValue, 2};
  boost::json::string_view value_part_2{kDataHandleValue + value_part_1.size(),
                                        3};
  boost::json::string_view value_part_last{
      kDataHandleValue + value_part_1.size() + value_part_2.size(),
      len(kDataHandleValue) - value_part_1.size() - value_part_2.size()};
  ASSERT_TRUE(
      handler.on_string_part(value_part_1, value_part_1.size(), error_code));
  ASSERT_TRUE(handler.on_string_part(
      value_part_2, value_part_1.size() + value_part_2.size(), error_code));
  ASSERT_TRUE(
      handler.on_string(value_part_last, len(kDataHandleValue), error_code));

  ASSERT_TRUE(handler.on_key(kPartition, len(kPartition), error_code));
  ASSERT_TRUE(
      handler.on_string(kPartitionValue, len(kPartitionValue), error_code));
  ASSERT_TRUE(handler.on_key(kChecksum, len(kChecksum), error_code));
  ASSERT_TRUE(
      handler.on_string(kChecksumValue, len(kChecksumValue), error_code));
  ASSERT_TRUE(handler.on_key(kDataSize, len(kDataSize), error_code));
  ASSERT_TRUE(handler.on_uint64(150, "150", error_code));
  ASSERT_TRUE(handler.on_key(kCompressedDataSize, len(kCompressedDataSize),
                             error_code));
  ASSERT_TRUE(handler.on_uint64(100, "100", error_code));
  ASSERT_TRUE(handler.on_key(kVersion, len(kVersion), error_code));
  ASSERT_TRUE(handler.on_uint64(6, "6", error_code));
  ASSERT_TRUE(handler.on_key(kCrc, len(kCrc), error_code));
  ASSERT_TRUE(handler.on_string(kCrcValue, len(kCrcValue), error_code));
  ASSERT_TRUE(handler.on_object_end(0, error_code));

  ASSERT_TRUE(handler.on_array_end(0, error_code));
  ASSERT_TRUE(handler.on_object_end(0, error_code));

  EXPECT_EQ(parsed_partition.GetDataHandle(), std::string(kDataHandleValue));
  EXPECT_EQ(parsed_partition.GetPartition(), std::string(kPartitionValue));
  EXPECT_EQ(olp::porting::value_or(parsed_partition.GetChecksum(), ""),
            std::string(kChecksumValue));
  EXPECT_EQ(olp::porting::value_or(parsed_partition.GetCrc(), ""),
            std::string(kCrcValue));
  EXPECT_EQ(olp::porting::value_or(parsed_partition.GetDataSize(), 0), 150);
  EXPECT_EQ(olp::porting::value_or(parsed_partition.GetCompressedDataSize(), 0),
            100);
  EXPECT_EQ(olp::porting::value_or(parsed_partition.GetVersion(), 0), 6);
}

TEST(PartitionsSaxHandlerTest, WrongJSONStructure) {
  auto callback = [&](model::Partition) {};

  repository::PartitionsSaxHandler handler(callback);
  boost::json::error_code error_code;

  // Initial state expects an object
  ASSERT_FALSE(handler.on_key(kPartitions, len(kPartitions), error_code));
  ASSERT_FALSE(handler.on_uint64(6, "6", error_code));
  ASSERT_FALSE(handler.on_array_begin(error_code));
  ASSERT_FALSE(handler.on_array_end(0, error_code));
  ASSERT_FALSE(handler.on_object_end(0, error_code));

  ASSERT_TRUE(handler.on_object_begin(error_code));

  // next state expects a partitions string
  ASSERT_FALSE(handler.on_key(kDataHandle, len(kDataHandle), error_code));
  ASSERT_FALSE(handler.on_uint64(6, "6", error_code));
  ASSERT_FALSE(handler.on_array_begin(error_code));
  ASSERT_FALSE(handler.on_object_begin(error_code));
  ASSERT_FALSE(handler.on_object_end(0, error_code));
  ASSERT_FALSE(handler.on_array_end(0, error_code));

  ASSERT_TRUE(handler.on_key(kPartitions, len(kPartitions), error_code));

  // expect partitions array
  ASSERT_FALSE(handler.on_key(kDataHandle, len(kDataHandle), error_code));
  ASSERT_FALSE(handler.on_uint64(6, "6", error_code));
  ASSERT_FALSE(handler.on_object_begin(error_code));
  ASSERT_FALSE(handler.on_object_end(0, error_code));
  ASSERT_FALSE(handler.on_array_end(0, error_code));

  ASSERT_TRUE(handler.on_array_begin(error_code));

  // expect partition object
  ASSERT_FALSE(handler.on_key(kDataHandle, len(kDataHandle), error_code));
  ASSERT_FALSE(handler.on_uint64(6, "6", error_code));
  ASSERT_FALSE(handler.on_array_begin(error_code));
  ASSERT_FALSE(handler.on_object_end(0, error_code));

  ASSERT_TRUE(handler.on_object_begin(error_code));

  // object is not valid
  ASSERT_FALSE(handler.on_object_end(0, error_code));

  // expect partition attribute
  ASSERT_FALSE(handler.on_uint64(6, "6", error_code));
  ASSERT_FALSE(handler.on_array_begin(error_code));

  ASSERT_TRUE(handler.on_key(kDataHandle, len(kDataHandle), error_code));

  // expect string attribute value
  ASSERT_FALSE(handler.on_uint64(6, "6", error_code));
  ASSERT_FALSE(handler.on_array_begin(error_code));
  ASSERT_FALSE(handler.on_object_end(0, error_code));

  ASSERT_TRUE(
      handler.on_string(kDataHandleValue, len(kDataHandleValue), error_code));

  // object is not valid
  ASSERT_FALSE(handler.on_object_end(0, error_code));

  // integer properties
  ASSERT_TRUE(handler.on_key(kDataSize, len(kDataSize), error_code));

  ASSERT_FALSE(handler.on_string(kDataHandle, len(kDataHandle), error_code));
  ASSERT_FALSE(handler.on_array_begin(error_code));
  ASSERT_FALSE(handler.on_array_end(0, error_code));
  ASSERT_FALSE(handler.on_object_begin(error_code));
  ASSERT_FALSE(handler.on_object_end(0, error_code));

  ASSERT_TRUE(handler.on_uint64(6, "6", error_code));

  ASSERT_TRUE(handler.on_key(kPartition, len(kPartition), error_code));
  ASSERT_TRUE(
      handler.on_string(kPartitionValue, len(kPartitionValue), error_code));

  // complete partition
  ASSERT_TRUE(handler.on_object_end(0, error_code));

  // complete partitions array
  ASSERT_TRUE(handler.on_array_end(0, error_code));

  // complete the json
  ASSERT_TRUE(handler.on_object_end(0, error_code));

  // nothing works anymore
  ASSERT_FALSE(handler.on_key(kDataHandle, len(kDataHandle), error_code));
  ASSERT_FALSE(handler.on_uint64(6, "6", error_code));
  ASSERT_FALSE(handler.on_array_begin(error_code));
  ASSERT_FALSE(handler.on_array_end(0, error_code));
  ASSERT_FALSE(handler.on_object_begin(error_code));
  ASSERT_FALSE(handler.on_object_end(0, error_code));
}

}  // namespace
