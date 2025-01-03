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

  repository::PartitionsSaxHandler handler(callback);

  ASSERT_TRUE(handler.StartObject());
  ASSERT_TRUE(handler.String(kPartitions, len(kPartitions), true));
  ASSERT_TRUE(handler.StartArray());

  ASSERT_TRUE(handler.StartObject());
  ASSERT_TRUE(handler.String(kDataHandle, len(kDataHandle), true));
  ASSERT_TRUE(handler.String(kDataHandleValue, len(kDataHandleValue), true));
  ASSERT_TRUE(handler.String(kPartition, len(kPartition), true));
  ASSERT_TRUE(handler.String(kPartitionValue, len(kPartitionValue), true));
  ASSERT_TRUE(handler.String(kChecksum, len(kChecksum), true));
  ASSERT_TRUE(handler.String(kChecksumValue, len(kChecksumValue), true));
  ASSERT_TRUE(handler.String(kDataSize, len(kDataSize), true));
  ASSERT_TRUE(handler.Uint(150));
  ASSERT_TRUE(handler.String(kCompressedDataSize, len(kCompressedDataSize), true));
  ASSERT_TRUE(handler.Uint(100));
  ASSERT_TRUE(handler.String(kVersion, len(kVersion), true));
  ASSERT_TRUE(handler.Uint(6));
  ASSERT_TRUE(handler.String(kCrc, len(kCrc), true));
  ASSERT_TRUE(handler.String(kCrcValue, len(kCrcValue), true));
  ASSERT_TRUE(handler.EndObject(0));

  ASSERT_TRUE(handler.EndArray(0));
  ASSERT_TRUE(handler.EndObject(0));

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

  repository::PartitionsSaxHandler handler(callback);

  // Initial state expects an object
  ASSERT_FALSE(handler.String(kPartitions, len(kPartitions), true));
  ASSERT_FALSE(handler.Uint(6));
  ASSERT_FALSE(handler.StartArray());
  ASSERT_FALSE(handler.EndArray(0));
  ASSERT_FALSE(handler.EndObject(0));

  ASSERT_TRUE(handler.StartObject());

  // next state expects a partitions string
  ASSERT_FALSE(handler.String(kDataHandle, len(kDataHandle), true));
  ASSERT_FALSE(handler.Uint(6));
  ASSERT_FALSE(handler.StartArray());
  ASSERT_FALSE(handler.StartObject());
  ASSERT_FALSE(handler.EndObject(0));
  ASSERT_FALSE(handler.EndArray(0));

  ASSERT_TRUE(handler.String(kPartitions, len(kPartitions), true));

  // expect partitions array
  ASSERT_FALSE(handler.String(kDataHandle, len(kDataHandle), true));
  ASSERT_FALSE(handler.Uint(6));
  ASSERT_FALSE(handler.StartObject());
  ASSERT_FALSE(handler.EndObject(0));
  ASSERT_FALSE(handler.EndArray(0));

  ASSERT_TRUE(handler.StartArray());

  // expect partition object
  ASSERT_FALSE(handler.String(kDataHandle, len(kDataHandle), true));
  ASSERT_FALSE(handler.Uint(6));
  ASSERT_FALSE(handler.StartArray());
  ASSERT_FALSE(handler.EndObject(0));

  ASSERT_TRUE(handler.StartObject());

  // object is not valid
  ASSERT_FALSE(handler.EndObject(0));

  // expect partition attribute
  ASSERT_FALSE(handler.Uint(6));
  ASSERT_FALSE(handler.StartArray());

  ASSERT_TRUE(handler.String(kDataHandle, len(kDataHandle), true));

  // expect string attribute value
  ASSERT_FALSE(handler.Uint(6));
  ASSERT_FALSE(handler.StartArray());
  ASSERT_FALSE(handler.EndObject(0));

  ASSERT_TRUE(handler.String(kDataHandleValue, len(kDataHandleValue), true));

  // object is not valid
  ASSERT_FALSE(handler.EndObject(0));

  // integer properties
  ASSERT_TRUE(handler.String(kDataSize, len(kDataSize), true));

  ASSERT_FALSE(handler.String(kDataHandle, len(kDataHandle), true));
  ASSERT_FALSE(handler.StartArray());
  ASSERT_FALSE(handler.EndArray(0));
  ASSERT_FALSE(handler.StartObject());
  ASSERT_FALSE(handler.EndObject(0));

  ASSERT_TRUE(handler.Uint(6));

  ASSERT_TRUE(handler.String(kPartition, len(kPartition), true));
  ASSERT_TRUE(handler.String(kPartitionValue, len(kPartitionValue), true));

  // complete partition
  ASSERT_TRUE(handler.EndObject(0));

  // complete partitions array
  ASSERT_TRUE(handler.EndArray(0));

  // complete the json
  ASSERT_TRUE(handler.EndObject(0));

  // nothing works anymore
  ASSERT_FALSE(handler.String(kDataHandle, len(kDataHandle), true));
  ASSERT_FALSE(handler.Uint(6));
  ASSERT_FALSE(handler.StartArray());
  ASSERT_FALSE(handler.EndArray(0));
  ASSERT_FALSE(handler.StartObject());
  ASSERT_FALSE(handler.EndObject(0));
}

}  // namespace
