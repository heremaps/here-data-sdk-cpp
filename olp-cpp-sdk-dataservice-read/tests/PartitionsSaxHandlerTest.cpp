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
  auto callback = [&](model::Partition partition) {
    parsed_partition = partition;
  };

  boost::json::error_code ec;
  repository::PartitionsSaxHandler handler(callback);

  ASSERT_TRUE(handler.on_object_begin(ec));
  ASSERT_TRUE(handler.on_key(kPartitions, len(kPartitions), ec));
  ASSERT_TRUE(handler.on_array_begin(ec));

  ASSERT_TRUE(handler.on_object_begin(ec));
  ASSERT_TRUE(handler.on_key(kDataHandle, len(kDataHandle), ec));
  ASSERT_TRUE(handler.on_string(kDataHandleValue, len(kDataHandleValue), ec));
  ASSERT_TRUE(handler.on_key(kPartition, len(kPartition), ec));
  ASSERT_TRUE(handler.on_string(kPartitionValue, len(kPartitionValue), ec));
  ASSERT_TRUE(handler.on_key(kChecksum, len(kChecksum), ec));
  ASSERT_TRUE(handler.on_string(kChecksumValue, len(kChecksumValue), ec));
  ASSERT_TRUE(handler.on_key(kDataSize, len(kDataSize), ec));
  ASSERT_TRUE(handler.on_uint64(150, {}, ec));
  ASSERT_TRUE(
      handler.on_key(kCompressedDataSize, len(kCompressedDataSize), ec));
  ASSERT_TRUE(handler.on_uint64(100, {}, ec));
  ASSERT_TRUE(handler.on_key(kVersion, len(kVersion), ec));
  ASSERT_TRUE(handler.on_uint64(6, {}, ec));
  ASSERT_TRUE(handler.on_key(kCrc, len(kCrc), ec));
  ASSERT_TRUE(handler.on_string(kCrcValue, len(kCrcValue), ec));
  ASSERT_TRUE(handler.on_object_end(0, ec));

  ASSERT_TRUE(handler.on_array_end(0, ec));
  ASSERT_TRUE(handler.on_object_end(0, ec));

  EXPECT_EQ(parsed_partition.GetDataHandle(), std::string(kDataHandleValue));
  EXPECT_EQ(parsed_partition.GetPartition(), std::string(kPartitionValue));
  EXPECT_EQ(parsed_partition.GetChecksum().get_value_or(""),
            std::string(kChecksumValue));
  EXPECT_EQ(parsed_partition.GetCrc().get_value_or(""), std::string(kCrcValue));
  EXPECT_EQ(parsed_partition.GetDataSize().get_value_or(0), 150);
  EXPECT_EQ(parsed_partition.GetCompressedDataSize().get_value_or(0), 100);
  EXPECT_EQ(parsed_partition.GetVersion().get_value_or(0), 6);
}

TEST(PartitionsSaxHandlerTest, WrongJSONStructure) {
  auto callback = [&](model::Partition) {};

  boost::json::error_code ec;
  repository::PartitionsSaxHandler handler(callback);

  // Initial state expects an object
  ASSERT_FALSE(handler.on_key(kPartitions, len(kPartitions), ec));
  ASSERT_FALSE(handler.on_uint64(6, {}, ec));
  ASSERT_FALSE(handler.on_array_begin(ec));
  ASSERT_FALSE(handler.on_array_end(0, ec));
  ASSERT_FALSE(handler.on_object_end(0, ec));

  ASSERT_TRUE(handler.on_object_begin(ec));

  // next state expects a partitions string
  ASSERT_FALSE(handler.on_key(kDataHandle, len(kDataHandle), ec));
  ASSERT_FALSE(handler.on_uint64(6, {}, ec));
  ASSERT_FALSE(handler.on_array_begin(ec));
  ASSERT_FALSE(handler.on_object_begin(ec));
  ASSERT_FALSE(handler.on_object_end(0, ec));
  ASSERT_FALSE(handler.on_array_end(0, ec));

  ASSERT_TRUE(handler.on_key(kPartitions, len(kPartitions), ec));

  // expect partitions array
  ASSERT_FALSE(handler.on_key(kDataHandle, len(kDataHandle), ec));
  ASSERT_FALSE(handler.on_uint64(6, {}, ec));
  ASSERT_FALSE(handler.on_object_begin(ec));
  ASSERT_FALSE(handler.on_object_end(0, ec));
  ASSERT_FALSE(handler.on_array_end(0, ec));

  ASSERT_TRUE(handler.on_array_begin(ec));

  // expect partition object
  ASSERT_FALSE(handler.on_key(kDataHandle, len(kDataHandle), ec));
  ASSERT_FALSE(handler.on_uint64(6, {}, ec));
  ASSERT_FALSE(handler.on_array_begin(ec));
  ASSERT_FALSE(handler.on_object_end(0, ec));

  ASSERT_TRUE(handler.on_object_begin(ec));

  // object is not valid
  ASSERT_FALSE(handler.on_object_end(0, ec));

  // expect partition attribute
  ASSERT_FALSE(handler.on_uint64(6, {}, ec));
  ASSERT_FALSE(handler.on_array_begin(ec));

  ASSERT_TRUE(handler.on_key(kDataHandle, len(kDataHandle), ec));

  // expect string attribute value
  ASSERT_FALSE(handler.on_uint64(6, {}, ec));
  ASSERT_FALSE(handler.on_array_begin(ec));
  ASSERT_FALSE(handler.on_object_end(0, ec));

  ASSERT_TRUE(handler.on_string(kDataHandleValue, len(kDataHandleValue), ec));

  // object is not valid
  ASSERT_FALSE(handler.on_object_end(0, ec));

  // integer properties
  ASSERT_TRUE(handler.on_key(kDataSize, len(kDataSize), ec));

  ASSERT_FALSE(handler.on_key(kDataHandle, len(kDataHandle), ec));
  ASSERT_FALSE(handler.on_array_begin(ec));
  ASSERT_FALSE(handler.on_array_end(0, ec));
  ASSERT_FALSE(handler.on_object_begin(ec));
  ASSERT_FALSE(handler.on_object_end(0, ec));

  ASSERT_TRUE(handler.on_uint64(6, {}, ec));

  ASSERT_TRUE(handler.on_key(kPartition, len(kPartition), ec));
  ASSERT_TRUE(handler.on_string(kPartitionValue, len(kPartitionValue), ec));

  // complete partition
  ASSERT_TRUE(handler.on_object_end(0, ec));

  // complete partitions array
  ASSERT_TRUE(handler.on_array_end(0, ec));

  // complete the json
  ASSERT_TRUE(handler.on_object_end(0, ec));

  // nothing works anymore
  ASSERT_FALSE(handler.on_key(kDataHandle, len(kDataHandle), ec));
  ASSERT_FALSE(handler.on_uint64(6, {}, ec));
  ASSERT_FALSE(handler.on_array_begin(ec));
  ASSERT_FALSE(handler.on_array_end(0, ec));
  ASSERT_FALSE(handler.on_object_begin(ec));
  ASSERT_FALSE(handler.on_object_end(0, ec));
}

}  // namespace
