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

#include <regex>
#include <string>

#include <gtest/gtest.h>

// clang-format off
// Ordering Required - Parser template specializations before JsonParser.h
#include <generated/serializer/PublicationSerializer.h>
#include <generated/serializer/PublishPartitionSerializer.h>
#include <generated/serializer/PublishPartitionsSerializer.h>
#include <generated/serializer/PublishDataRequestSerializer.h>
#include <generated/serializer/JsonSerializer.h>
// clang-format on

namespace {

using namespace olp::dataservice::write::model;

void RemoveWhitespaceAndNewlines(std::string& s) {
  std::regex r("\\s+");
  s = std::regex_replace(s, r, "");
}

TEST(SerializerTest, Publication) {
  Publication publication;
  publication.SetId("34bc2a16-0373-4157-8ccc-19ba08a6672b");

  Details details;
  details.SetState("initialized");
  details.SetMessage("Publication initialized");
  details.SetStarted(1523459129829);
  details.SetModified(1523459129829);
  details.SetExpires(1523459129829);
  publication.SetDetails(details);
  publication.SetLayerIds({"my-layer"});

  VersionDependency version_dependency;
  version_dependency.SetDirect(true);
  version_dependency.SetHrn("hrn:here:data:::my-catalog");
  version_dependency.SetVersion(1);
  publication.SetVersionDependencies({version_dependency});
  publication.SetCatalogVersion(1);

  auto json = olp::serializer::serialize<Publication>(publication);

  std::string valid_json = R"(
      {
        "id": "34bc2a16-0373-4157-8ccc-19ba08a6672b",
        "details": {
          "state": "initialized",
          "message": "Publication initialized",
          "started": 1523459129829,
          "modified": 1523459129829,
          "expires": 1523459129829
        },
        "layerIds": [
          "my-layer"
        ],
        "catalogVersion": 1,
        "versionDependencies": [
          {
            "direct": true,
            "hrn": "hrn:here:data:::my-catalog",
            "version": 1
          }
        ]
      }
)";

  RemoveWhitespaceAndNewlines(valid_json);
  RemoveWhitespaceAndNewlines(json);

  EXPECT_EQ(valid_json, json);
}

TEST(SerializerTest, PublicationOnlyLayerIds) {
  Publication publication;

  publication.SetLayerIds({"my-layer"});

  auto json = olp::serializer::serialize<Publication>(publication);

  std::string valid_json = R"(
      {
        "layerIds": [
          "my-layer"
        ]
      }
)";

  RemoveWhitespaceAndNewlines(valid_json);
  RemoveWhitespaceAndNewlines(json);

  EXPECT_EQ(valid_json, json);
}

TEST(SerializerTest, PublishPartition) {
  PublishPartition publish_partition;

  publish_partition.SetPartition("314010583");
  publish_partition.SetChecksum("ff7494d6f17da702862e550c907c0a91");
  publish_partition.SetCompressedDataSize(152417);
  publish_partition.SetDataSize(250110);
  const char* buffer =
      "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAABGdBTUEAALGPC/"
      "xhBQAAABhQTFRFvb29AACEAP8AhIKEPb5x2m9E5413aFQirhRuvAMqCw+"
      "6kE2BVsa8miQaYSKyshxFvhqdzKx8UsPYk9gDEcY1ghZXcPbENtax8g5T+"
      "3zHYufF1Lf9HdIZBfNEiKAAAAAElFTkSuQmCC";
  publish_partition.SetData(std::make_shared<std::vector<unsigned char>>(
      buffer, buffer + strlen(buffer)));
  publish_partition.SetDataHandle("1b2ca68f-d4a0-4379-8120-cd025640510c");
  publish_partition.SetTimestamp(1519219235);

  auto json = olp::serializer::serialize<PublishPartition>(publish_partition);

  std::string valid_json = R"(
      {
        "partition": "314010583",
        "checksum": "ff7494d6f17da702862e550c907c0a91",
        "compressedDataSize": 152417,
        "dataSize": 250110,
        "data": "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAABGdBTUEAALGPC/xhBQAAABhQTFRFvb29AACEAP8AhIKEPb5x2m9E5413aFQirhRuvAMqCw+6kE2BVsa8miQaYSKyshxFvhqdzKx8UsPYk9gDEcY1ghZXcPbENtax8g5T+3zHYufF1Lf9HdIZBfNEiKAAAAAElFTkSuQmCC",
        "dataHandle": "1b2ca68f-d4a0-4379-8120-cd025640510c",
        "timestamp": 1519219235
      }
)";

  RemoveWhitespaceAndNewlines(valid_json);
  RemoveWhitespaceAndNewlines(json);

  EXPECT_EQ(valid_json, json);
}

TEST(SerializerTest, PublishPartitions) {
  PublishPartitions publish_partitions;
  PublishPartition publish_partition;

  publish_partition.SetPartition("314010583");
  publish_partition.SetChecksum("ff7494d6f17da702862e550c907c0a91");
  publish_partition.SetCompressedDataSize(152417);
  publish_partition.SetDataSize(250110);
  const char* buffer =
      "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAABGdBTUEAALGPC/"
      "xhBQAAABhQTFRFvb29AACEAP8AhIKEPb5x2m9E5413aFQirhRuvAMqCw+"
      "6kE2BVsa8miQaYSKyshxFvhqdzKx8UsPYk9gDEcY1ghZXcPbENtax8g5T+"
      "3zHYufF1Lf9HdIZBfNEiKAAAAAElFTkSuQmCC";
  publish_partition.SetData(std::make_shared<std::vector<unsigned char>>(
      buffer, buffer + strlen(buffer)));
  publish_partition.SetDataHandle("1b2ca68f-d4a0-4379-8120-cd025640510c");
  publish_partition.SetTimestamp(1519219235);

  publish_partitions.SetPartitions({publish_partition});

  auto json = olp::serializer::serialize<PublishPartitions>(publish_partitions);

  std::string valid_json = R"(
    {
      "partitions": [
        {
          "partition": "314010583",
          "checksum": "ff7494d6f17da702862e550c907c0a91",
          "compressedDataSize": 152417,
          "dataSize": 250110,
          "data": "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAABGdBTUEAALGPC/xhBQAAABhQTFRFvb29AACEAP8AhIKEPb5x2m9E5413aFQirhRuvAMqCw+6kE2BVsa8miQaYSKyshxFvhqdzKx8UsPYk9gDEcY1ghZXcPbENtax8g5T+3zHYufF1Lf9HdIZBfNEiKAAAAAElFTkSuQmCC",
          "dataHandle": "1b2ca68f-d4a0-4379-8120-cd025640510c",
          "timestamp": 1519219235
        }
      ]
    }
)";

  RemoveWhitespaceAndNewlines(valid_json);
  RemoveWhitespaceAndNewlines(json);

  EXPECT_EQ(valid_json, json);
}

TEST(SerializerTest, PublishPartitionsOnlyPartitionAndDatahandle) {
  PublishPartitions publish_partitions;
  PublishPartition publish_partition;

  publish_partition.SetPartition("314010583");
  publish_partition.SetDataHandle("1b2ca68f-d4a0-4379-8120-cd025640510c");

  publish_partitions.SetPartitions({publish_partition});

  auto json = olp::serializer::serialize<PublishPartitions>(publish_partitions);

  std::string valid_json = R"(
    {
      "partitions": [
        {
          "partition": "314010583",
          "dataHandle": "1b2ca68f-d4a0-4379-8120-cd025640510c"
        }
      ]
    }
)";

  RemoveWhitespaceAndNewlines(valid_json);
  RemoveWhitespaceAndNewlines(json);

  EXPECT_EQ(valid_json, json);
}

TEST(SerializerTest, PublishDataRequest) {
  PublishDataRequest publish_data_request;

  std::string data_string = "payload";
  std::shared_ptr<std::vector<unsigned char>> data_ =
      std::make_shared<std::vector<unsigned char>>(data_string.begin(),
                                                   data_string.end());
  publish_data_request.WithBillingTag("OlpCppSdkTest")
      .WithChecksum("olp-cpp-sdk-checksum")
      .WithData(data_)
      .WithLayerId("olp-cpp-sdk-layer")
      .WithTraceId("04946af8-7f0e-4d41-b85a-e883c74ebba3");

  auto json =
      olp::serializer::serialize<PublishDataRequest>(publish_data_request);

  std::string valid_json = R"(
      {
        "data": "payload",
        "layerId": "olp-cpp-sdk-layer",
        "traceId": "04946af8-7f0e-4d41-b85a-e883c74ebba3",
        "billingTag": "OlpCppSdkTest",
        "checksum": "olp-cpp-sdk-checksum"
      }
)";

  RemoveWhitespaceAndNewlines(valid_json);
  RemoveWhitespaceAndNewlines(json);

  EXPECT_EQ(valid_json, json);
}

}  // namespace
