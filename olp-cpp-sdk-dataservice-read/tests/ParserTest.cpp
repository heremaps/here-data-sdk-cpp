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

#include <chrono>
#include <string>

#include <gtest/gtest.h>
// clang-format off
// this order is required
#include "generated/parser/ApiParser.h"
#include "generated/parser/CatalogParser.h"
#include "generated/parser/LayerVersionsParser.h"
#include "generated/parser/PartitionsParser.h"
#include "generated/parser/VersionResponseParser.h"
#include "generated/parser/IndexParser.h"
#include "generated/parser/MessagesParser.h"
#include "generated/parser/SubscribeResponseParser.h"
#include <olp/core/generated/parser/JsonParser.h>
// clang-format on

namespace {

TEST(ParserTest, Api) {
  std::string json_input =
      "[\
    {\
    \"api\": \"config\",\
    \"version\": \"v1\",\
    \"baseURL\": \"https://config.data.api.platform.here.com/config/v1\",\
    \"parameters\": {\
    \"additionalProp1\": \"string\",\
    \"additionalProp2\": \"string\",\
    \"additionalProp3\": \"string\"\
    }\
    }\
    ]";

  auto start_time = std::chrono::high_resolution_clock::now();
  auto apis =
      olp::parser::parse<olp::dataservice::read::model::Apis>(json_input);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_EQ(1u, apis.size());
  ASSERT_EQ("config", apis.at(0).GetApi());
  ASSERT_EQ("v1", apis.at(0).GetVersion());
  ASSERT_EQ("https://config.data.api.platform.here.com/config/v1",
            apis.at(0).GetBaseUrl());
  ASSERT_EQ(3u, apis.at(0).GetParameters().size());
  ASSERT_EQ("string", apis.at(0).GetParameters().at("additionalProp1"));
  ASSERT_EQ("string", apis.at(0).GetParameters().at("additionalProp2"));
  ASSERT_EQ("string", apis.at(0).GetParameters().at("additionalProp3"));
}

TEST(ParserTest, Catalog) {
  std::string json_input =
      "{\
    \"id\": \"roadweather-catalog-v1\",\
    \"hrn\": \"hrn:here:data::olp-here-test:my-catalog-v1\",\
    \"name\": \"string\",\
    \"summary\": \"Contains estimates for road conditions based on weather data.\",\
    \"description\": \"Road conditions are typically based on the temperature, comfort level, wind speed and direction. However, other weather-based data points can be taken into account.\",\
    \"coverage\": {\
    \"adminAreas\": [\
    \"DE\"\
    ]\
    },\
    \"owner\": {\
    \"creator\": {\
    \"id\": \"string\"\
    },\
    \"organisation\": {\
    \"id\": \"HERE\"\
    }\
    },\
    \"tags\": [\
    \"Roads\",\
    \"Weather\"\
    ],\
    \"billingTags\": [\
    \"Cost Center 1\",\
    \"Cost Center 2\"\
    ],\
    \"created\": \"2017-08-04T17:19:03.853Z\",\
    \"layers\": [\
    {\
    \"id\": \"traffic-incidents\",\
    \"name\": \"Traffic Incidents\",\
    \"summary\": \"This layer provides aggregated information about traffic incidents.\",\
    \"description\": \"This layer provides aggregated information about traffic incidents, including the type and location of each traffic incident, status, start and end time, and other relevant data. This data is useful to dynamically optimize route calculations.\",\
    \"owner\": {\
    \"creator\": {\
    \"id\": \"string\"\
    },\
    \"organisation\": {\
    \"id\": \"HERE\"\
    }\
    },\
    \"coverage\": {\
    \"adminAreas\": [\
    \"DE\"\
    ]\
    },\
    \"schema\": {\
    \"hrn\": \"hrn:here:schema:::com.here.schema.rib:topology-geometry_v2:2.2.0\"\
    },\
    \"contentType\": \"application/json\",\
    \"contentEncoding\": \"gzip\",\
    \"partitioning\": {\
    \"scheme\": \"heretile\",\
    \"tileLevels\": [\
    12\
    ]\
    },\
    \"layerType\": \"versioned\",\
    \"digest\": \"SHA-1\",\
    \"tags\": [\
    \"Roads\",\
    \"Weather\"\
    ],\
    \"billingTags\": [\
    \"Cost Center 1\",\
    \"Cost Center 2\"\
    ],\
    \"ttl\": 24,\
    \"indexProperties\": {\
    \"ttl\": \"1.year\",\
    \"indexDefinitions\": [\
    {\
    \"name\": \"string\",\
    \"type\": \"bool\",\
    \"duration\": 0,\
    \"zoomLevel\": 0\
    }\
    ]\
    },\
    \"streamProperties\": {\
    \"dataInThroughputMbps\": 10,\
    \"dataOutThroughputMbps\": 10\
    },\
    \"volume\": {\
    \"volumeType\": \"durable\",\
    \"maxMemoryPolicy\": \"failOnWrite\",\
    \"packageType\": \"small\",\
    \"encryption\": {\
    \"algorithm\": \"aes256\"\
    }\
    }\
    }\
    ],\
    \"version\": 1,\
    \"notifications\": {\
    \"enabled\": false\
    }\
    }";

  auto start_time = std::chrono::high_resolution_clock::now();
  auto catalog =
      olp::parser::parse<olp::dataservice::read::model::Catalog>(json_input);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;

  ASSERT_EQ("roadweather-catalog-v1", catalog.GetId());
  ASSERT_EQ("hrn:here:data::olp-here-test:my-catalog-v1", catalog.GetHrn());
  ASSERT_EQ("string", catalog.GetName());
  ASSERT_EQ("Contains estimates for road conditions based on weather data.",
            catalog.GetSummary());
  ASSERT_EQ(
      "Road conditions are typically based on the temperature, comfort level, "
      "wind speed and "
      "direction. However, other weather-based data points can be taken into "
      "account.",
      catalog.GetDescription());
  ASSERT_EQ(1u, catalog.GetCoverage().GetAdminAreas().size());
  ASSERT_EQ("DE", catalog.GetCoverage().GetAdminAreas().at(0));
  ASSERT_EQ("string", catalog.GetOwner().GetCreator().GetId());
  ASSERT_EQ("HERE", catalog.GetOwner().GetOrganisation().GetId());
  ASSERT_EQ(2u, catalog.GetTags().size());
  ASSERT_EQ("Roads", catalog.GetTags().at(0));
  ASSERT_EQ("Weather", catalog.GetTags().at(1));
  ASSERT_EQ(2u, catalog.GetBillingTags().size());
  ASSERT_EQ("Cost Center 1", catalog.GetBillingTags().at(0));
  ASSERT_EQ("Cost Center 2", catalog.GetBillingTags().at(1));
  ASSERT_EQ("2017-08-04T17:19:03.853Z", catalog.GetCreated());
  ASSERT_EQ(1, catalog.GetVersion());
  ASSERT_EQ(false, catalog.GetNotifications().GetEnabled());
  ASSERT_EQ(1u, catalog.GetLayers().size());
  ASSERT_EQ("traffic-incidents", catalog.GetLayers().at(0).GetId());
  ASSERT_EQ("Traffic Incidents", catalog.GetLayers().at(0).GetName());
  ASSERT_EQ(
      "This layer provides aggregated information about traffic incidents.",
      catalog.GetLayers().at(0).GetSummary());
  ASSERT_EQ(
      "This layer provides aggregated information about traffic incidents, "
      "including the type "
      "and location of each traffic incident, status, start and end time, and "
      "other relevant "
      "data. This data is useful to dynamically optimize route calculations.",
      catalog.GetLayers().at(0).GetDescription());
  ASSERT_EQ("string",
            catalog.GetLayers().at(0).GetOwner().GetCreator().GetId());
  ASSERT_EQ("HERE",
            catalog.GetLayers().at(0).GetOwner().GetOrganisation().GetId());
  ASSERT_EQ(1u, catalog.GetLayers().at(0).GetCoverage().GetAdminAreas().size());
  ASSERT_EQ("DE",
            catalog.GetLayers().at(0).GetCoverage().GetAdminAreas().at(0));
  ASSERT_EQ("hrn:here:schema:::com.here.schema.rib:topology-geometry_v2:2.2.0",
            catalog.GetLayers().at(0).GetSchema().GetHrn());
  ASSERT_EQ("application/json", catalog.GetLayers().at(0).GetContentType());
  ASSERT_EQ("gzip", catalog.GetLayers().at(0).GetContentEncoding());
  ASSERT_EQ("heretile",
            catalog.GetLayers().at(0).GetPartitioning().GetScheme());
  ASSERT_EQ(1u,
            catalog.GetLayers().at(0).GetPartitioning().GetTileLevels().size());
  ASSERT_EQ(12ll,
            catalog.GetLayers().at(0).GetPartitioning().GetTileLevels().at(0));
  ASSERT_EQ("versioned", catalog.GetLayers().at(0).GetLayerType());
  ASSERT_EQ("SHA-1", catalog.GetLayers().at(0).GetDigest());
  ASSERT_EQ(2u, catalog.GetLayers().at(0).GetTags().size());
  ASSERT_EQ("Roads", catalog.GetLayers().at(0).GetTags().at(0));
  ASSERT_EQ("Weather", catalog.GetLayers().at(0).GetTags().at(1));
  ASSERT_EQ(2u, catalog.GetLayers().at(0).GetBillingTags().size());
  ASSERT_EQ("Cost Center 1", catalog.GetLayers().at(0).GetBillingTags().at(0));
  ASSERT_EQ("Cost Center 2", catalog.GetLayers().at(0).GetBillingTags().at(1));
  ASSERT_EQ(24, *catalog.GetLayers().at(0).GetTtl());
  ASSERT_EQ("1.year", catalog.GetLayers().at(0).GetIndexProperties().GetTtl());
  ASSERT_EQ(1u, catalog.GetLayers()
                    .at(0)
                    .GetIndexProperties()
                    .GetIndexDefinitions()
                    .size());
  ASSERT_EQ("string", catalog.GetLayers()
                          .at(0)
                          .GetIndexProperties()
                          .GetIndexDefinitions()
                          .at(0)
                          .GetName());
  ASSERT_EQ("bool", catalog.GetLayers()
                        .at(0)
                        .GetIndexProperties()
                        .GetIndexDefinitions()
                        .at(0)
                        .GetType());
  ASSERT_EQ(0, catalog.GetLayers()
                   .at(0)
                   .GetIndexProperties()
                   .GetIndexDefinitions()
                   .at(0)
                   .GetDuration());
  ASSERT_EQ(0, catalog.GetLayers()
                   .at(0)
                   .GetIndexProperties()
                   .GetIndexDefinitions()
                   .at(0)
                   .GetZoomLevel());
  ASSERT_EQ(10, catalog.GetLayers()
                    .at(0)
                    .GetStreamProperties()
                    .GetDataInThroughputMbps());
  ASSERT_EQ(10, catalog.GetLayers()
                    .at(0)
                    .GetStreamProperties()
                    .GetDataOutThroughputMbps());
  ASSERT_EQ("durable", catalog.GetLayers().at(0).GetVolume().GetVolumeType());
  ASSERT_EQ("failOnWrite",
            catalog.GetLayers().at(0).GetVolume().GetMaxMemoryPolicy());
  ASSERT_EQ("small", catalog.GetLayers().at(0).GetVolume().GetPackageType());
  ASSERT_EQ(
      "aes256",
      catalog.GetLayers().at(0).GetVolume().GetEncryption().GetAlgorithm());
}

TEST(ParserTest, Partitions) {
  std::string json_input =
      "{\
    \"partitions\": [\
      { \
        \"checksum\": \"291f66029c232400e3403cd6e9cfd36e\",\
        \"compressedDataSize\": 1024,\
        \"dataHandle\": \"1b2ca68f-d4a0-4379-8120-cd025640510c\",\
        \"dataSize\": 1024,\
        \"partition\": \"314010583\",\
        \"version\": 2\
      }\
    ],\
    \"next\": \"url\"\
  }";

  auto start_time = std::chrono::high_resolution_clock::now();
  auto partitions =
      olp::parser::parse<olp::dataservice::read::model::Partitions>(json_input);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;

  ASSERT_EQ(1u, partitions.GetPartitions().size());
  ASSERT_TRUE(partitions.GetPartitions().at(0).GetChecksum());
  ASSERT_EQ("291f66029c232400e3403cd6e9cfd36e",
            *partitions.GetPartitions().at(0).GetChecksum());
  ASSERT_TRUE(partitions.GetPartitions().at(0).GetCompressedDataSize());
  ASSERT_EQ(1024, *partitions.GetPartitions().at(0).GetCompressedDataSize());
  ASSERT_EQ("1b2ca68f-d4a0-4379-8120-cd025640510c",
            partitions.GetPartitions().at(0).GetDataHandle());
  ASSERT_TRUE(partitions.GetPartitions().at(0).GetDataSize());
  ASSERT_EQ(1024, *partitions.GetPartitions().at(0).GetDataSize());
  ASSERT_EQ("314010583", partitions.GetPartitions().at(0).GetPartition());
  ASSERT_TRUE(partitions.GetPartitions().at(0).GetVersion());
  ASSERT_EQ(2, *partitions.GetPartitions().at(0).GetVersion());
}

TEST(ParserTest, VersionResponse) {
  std::string json_input =
      "{\
    \"version\": 0\
    }";

  auto start_time = std::chrono::high_resolution_clock::now();
  auto version_response =
      olp::parser::parse<olp::dataservice::read::model::VersionResponse>(
          json_input);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;

  ASSERT_EQ(0, version_response.GetVersion());
}

TEST(ParserTest, LayerVersions) {
  std::string json_input =
      "{\
      \"layerVersions\": [\
        {\
          \"layer\": \"my-layer\",\
          \"version\": 0,\
          \"timestamp\": 1516397474657\
        }\
      ],\
      \"version\": 1\
    }";

  auto start_time = std::chrono::high_resolution_clock::now();
  auto layer_versions =
      olp::parser::parse<olp::dataservice::read::model::LayerVersions>(
          json_input);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;

  ASSERT_EQ(1, layer_versions.GetVersion());
  ASSERT_EQ(1u, layer_versions.GetLayerVersions().size());
  ASSERT_EQ("my-layer", layer_versions.GetLayerVersions().at(0).GetLayer());
  ASSERT_EQ(0, layer_versions.GetLayerVersions().at(0).GetVersion());
  ASSERT_EQ(1516397474657,
            layer_versions.GetLayerVersions().at(0).GetTimestamp());
}

#define INDEX_JSON \
  R"jsonString({"subQuads": [{"version":1,"subQuadKey":"mandatory","dataHandle":"KRAt-LQ9c-w24d-O60T"},{"version":2,"subQuadKey":"optional","dataHandle":"pxFY-PlnK-DRBS-Is7F", "additionalMetadata": "add-meta", "checksum": "checksum", "compressedDataSize": 10101, "dataSize": 21212}],"parentQuads": [{"version":3,"partition":"mandatory-params","dataHandle":"KB6r-AEDB-owiq-BaWt"}, {"version":4,"partition":"optional-params","dataHandle":"KB6r-AEDB-owiq-BaWt", "additionalMetadata": "add-meta", "checksum": "checksum", "compressedDataSize": 10101, "dataSize": 21212}]})jsonString"

TEST(ParserTest, Index) {
  auto start_time = std::chrono::high_resolution_clock::now();
  auto index =
      olp::parser::parse<olp::dataservice::read::model::Index>(INDEX_JSON);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;

  ASSERT_EQ(2, index.GetParentQuads().size());
  {
    auto& mandatory = index.GetParentQuads().at(0);
    ASSERT_EQ(3, mandatory->GetVersion());
    ASSERT_EQ("mandatory-params", mandatory->GetPartition());
    ASSERT_EQ("KB6r-AEDB-owiq-BaWt", mandatory->GetDataHandle());

    ASSERT_FALSE(mandatory->GetAdditionalMetadata());
    ASSERT_FALSE(mandatory->GetChecksum());
    ASSERT_FALSE(mandatory->GetCompressedDataSize());
    ASSERT_FALSE(mandatory->GetDataSize());
  }

  {
    auto& optional = index.GetParentQuads().at(1);
    ASSERT_EQ(4, optional->GetVersion());
    ASSERT_EQ("optional-params", optional->GetPartition());
    ASSERT_EQ("KB6r-AEDB-owiq-BaWt", optional->GetDataHandle());

    ASSERT_TRUE(optional->GetAdditionalMetadata());
    ASSERT_TRUE(optional->GetChecksum());
    ASSERT_TRUE(optional->GetCompressedDataSize());
    ASSERT_TRUE(optional->GetDataSize());

    ASSERT_EQ("add-meta", optional->GetAdditionalMetadata().get());
    ASSERT_EQ("checksum", optional->GetChecksum().get());
    ASSERT_EQ(10101, optional->GetCompressedDataSize().get());
    ASSERT_EQ(21212, optional->GetDataSize().get());
  }

  ASSERT_EQ(2, index.GetSubQuads().size());
  {
    auto& mandatory = index.GetSubQuads().at(0);
    ASSERT_EQ(1, mandatory->GetVersion());
    ASSERT_EQ("mandatory", mandatory->GetSubQuadKey());
    ASSERT_EQ("KRAt-LQ9c-w24d-O60T", mandatory->GetDataHandle());

    ASSERT_FALSE(mandatory->GetAdditionalMetadata());
    ASSERT_FALSE(mandatory->GetChecksum());
    ASSERT_FALSE(mandatory->GetCompressedDataSize());
    ASSERT_FALSE(mandatory->GetDataSize());
  }

  {
    auto& optional = index.GetSubQuads().at(1);
    ASSERT_EQ(2, optional->GetVersion());
    ASSERT_EQ("optional", optional->GetSubQuadKey());
    ASSERT_EQ("pxFY-PlnK-DRBS-Is7F", optional->GetDataHandle());

    ASSERT_TRUE(optional->GetAdditionalMetadata());
    ASSERT_TRUE(optional->GetChecksum());
    ASSERT_TRUE(optional->GetCompressedDataSize());
    ASSERT_TRUE(optional->GetDataSize());

    ASSERT_EQ("add-meta", optional->GetAdditionalMetadata().get());
    ASSERT_EQ("checksum", optional->GetChecksum().get());
    ASSERT_EQ(10101, optional->GetCompressedDataSize().get());
    ASSERT_EQ(21212, optional->GetDataSize().get());
  }
}

TEST(ParserTest, Messages) {
  {
    const std::string kData =
        "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAABGdBTUEAALGPC/"
        "xhBQAAABhQTFRFvb29AACEAP8AhIKEPb5x2m9E5413aFQirhRuvAMqCw+"
        "6kE2BVsa8miQaYSKyshxFvhqdzKx8UsPYk9gDEcY1ghZXcPbENtax8g5T+"
        "3zHYufF1Lf9HdIZBfNEiKAAAAAElFTkSuQmCC";

    const std::string valid_message_json =
        "{\
      \"metaData\":{\
      \"partition\":\"314010583\",\
      \"checksum\":\"ff7494d6f17da702862e550c907c0a91\",\
      \"compressedDataSize\":152417,\
      \"dataSize\":250110,\
      \"data\":\"" +
        kData +
        "\",\
      \"dataHandle\":\"bb76b7747e7523596e74a138b15d92ec\",\
      \"timestamp\":1517916706\
      },\
    \"offset\":{\
    \"partition\":7,\
    \"offset\":38562\
      }}";
    const std::string invalid_message_json = "{\"some_invalid_json\":\"yes\"}";

    const std::string messages_json = "{\"messages\":[" + valid_message_json +
                                      "," + invalid_message_json + "]}";
    auto messages = olp::parser::parse<olp::dataservice::read::model::Messages>(
                        messages_json)
                        .GetMessages();
    ASSERT_EQ(2, messages.size());

    {
      SCOPED_TRACE("Parse valid message");

      const std::vector<unsigned char> kDataVec(kData.begin(), kData.end());
      auto valid_message = messages[0];

      {
        SCOPED_TRACE("Verify valid metadata");

        auto metadata = valid_message.GetMetaData();
        EXPECT_EQ(std::string("314010583"), metadata.GetPartition());

        ASSERT_TRUE(metadata.GetData() != nullptr);
        EXPECT_EQ(kDataVec, *metadata.GetData());

        // Verify optional fields:
        ASSERT_TRUE(metadata.GetChecksum());
        ASSERT_TRUE(metadata.GetCompressedDataSize());
        ASSERT_TRUE(metadata.GetDataSize());
        ASSERT_TRUE(metadata.GetDataHandle());
        ASSERT_TRUE(metadata.GetTimestamp());

        EXPECT_EQ(std::string("ff7494d6f17da702862e550c907c0a91"),
                  metadata.GetChecksum().get());
        EXPECT_EQ(152417, metadata.GetCompressedDataSize().get());
        EXPECT_EQ(250110, metadata.GetDataSize().get());
        EXPECT_EQ(std::string("bb76b7747e7523596e74a138b15d92ec"),
                  metadata.GetDataHandle().get());
        EXPECT_EQ(1517916706, metadata.GetTimestamp().get());
      }

      {
        SCOPED_TRACE("Verify valid data");

        ASSERT_TRUE(valid_message.GetData() != nullptr);
        EXPECT_EQ(kDataVec, *valid_message.GetData());
      }

      {
        SCOPED_TRACE("Verify valid stream offset");

        auto stream_offset = valid_message.GetOffset();
        EXPECT_EQ(7, stream_offset.GetPartition());
        EXPECT_EQ(38562, stream_offset.GetOffset());
      }
    }

    {
      SCOPED_TRACE("Parse invalid message");

      auto invalid_message = messages[1];
      {
        SCOPED_TRACE("Verify invalid metadata");

        auto metadata = invalid_message.GetMetaData();
        EXPECT_TRUE(metadata.GetPartition().empty());
        EXPECT_TRUE(metadata.GetData() == nullptr);

        // Verify invalid optional fields:
        EXPECT_FALSE(metadata.GetChecksum());
        EXPECT_FALSE(metadata.GetCompressedDataSize());
        EXPECT_FALSE(metadata.GetDataSize());
        EXPECT_FALSE(metadata.GetDataHandle());
        EXPECT_FALSE(metadata.GetTimestamp());
      }

      {
        SCOPED_TRACE("Verify invalid data");

        EXPECT_TRUE(invalid_message.GetData() == nullptr);
      }

      {
        SCOPED_TRACE("Verify invalid stream offset");

        auto stream_offset = invalid_message.GetOffset();
        EXPECT_EQ(0, stream_offset.GetPartition());
        EXPECT_EQ(0, stream_offset.GetOffset());
      }
    }
  }

  {
    SCOPED_TRACE("Parse invalid messages array");

    const std::string invalid_messages_json =
        "\"invalid_messages_array\":\"yes\"";
    auto messages = olp::parser::parse<olp::dataservice::read::model::Messages>(
                        invalid_messages_json)
                        .GetMessages();
    EXPECT_TRUE(messages.empty());
  }
}

TEST(ParserTest, SubscribeResponse) {
  {
    SCOPED_TRACE("Parse valid SubscribeResponse");
    const std::string kNodeBaseUrl =
        "https://some.stream.url/stream/catalog-id";
    const std::string kSubscriptionId =
        "-1920183912.123e4567-e89b-12d3-a456-556642440000";
    const std::string kValidSubscribeResponse =
        "{\
      \"nodeBaseURL\":\"" +
        kNodeBaseUrl + "\",\"subscriptionId\":\"" + kSubscriptionId + "\"}";

    const auto response =
        olp::parser::parse<olp::dataservice::read::model::SubscribeResponse>(
            kValidSubscribeResponse);
    EXPECT_EQ(kNodeBaseUrl, response.GetNodeBaseURL());
    EXPECT_EQ(kSubscriptionId, response.GetSubscriptionId());
  }

  {
    SCOPED_TRACE("Parse invalid SubscribeResponse");
    const std::string kInvalidSubscribeResponse =
        "{\
      \"invalid_nodeBaseURL\":\"some_url\",\
      \"invalid_subscriptionId\":\"42\"\
      }";

    const auto response =
        olp::parser::parse<olp::dataservice::read::model::SubscribeResponse>(
            kInvalidSubscribeResponse);
    EXPECT_TRUE(response.GetNodeBaseURL().empty());
    EXPECT_TRUE(response.GetSubscriptionId().empty());
  }
}

}  // namespace
