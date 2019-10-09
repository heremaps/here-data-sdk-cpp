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
// Ordering Required - Parser template specializations before JsonParser.h
#include <generated/parser/ApiParser.h>
#include <generated/parser/CatalogParser.h>
#include <generated/parser/DetailsParser.h>
#include <generated/parser/PublicationParser.h>
#include <generated/parser/PartitionsParser.h>
#include <generated/parser/PublishPartitionParser.h>
#include <generated/parser/PublishPartitionsParser.h>
#include <generated/parser/ResponseOkParser.h>
#include <generated/parser/ResponseOkSingleParser.h>
#include <generated/parser/VersionDependencyParser.h>
#include <generated/parser/LayerVersionsParser.h>
#include <generated/parser/PublishDataRequestParser.h>
#include <olp/core/generated/parser/JsonParser.h>
// clang-format on

namespace {

using namespace olp::dataservice::write::model;

TEST(ParserTest, ResponseOkSingle) {
  auto json = R"(
        {
            "TraceID": "835eb107-7a35-478f-b41c-dd8e750affe0"
        }
    )";

  auto response = olp::parser::parse<ResponseOkSingle>(json);

  EXPECT_STREQ(response.GetTraceID().c_str(),
               "835eb107-7a35-478f-b41c-dd8e750affe0");
}

TEST(ParserTest, ResponseOkOneGeneratedId) {
  auto json = R"(
        {
            "TraceID": {
                "ParentID": "66cab713-4576-4eef-b01b-ad088a1e3b82",
                "GeneratedIDs": [
                    "496546b2-04fd-4098-9419-c2fbe39a98a6"
                ]
            }
        }
    )";

  auto response = olp::parser::parse<ResponseOk>(json);

  EXPECT_STREQ(response.GetTraceID().GetParentID().c_str(),
               "66cab713-4576-4eef-b01b-ad088a1e3b82");

  ASSERT_EQ(response.GetTraceID().GetGeneratedIDs().size(), 1);
  EXPECT_STREQ(response.GetTraceID().GetGeneratedIDs().at(0).c_str(),
               "496546b2-04fd-4098-9419-c2fbe39a98a6");
}

TEST(ParserTest, ResponseOkMultipleGeneratedIds) {
  auto json = R"(
        {
            "TraceID": {
                "ParentID": "2e05aaee-4c02-4735-9dbf-27eba9a47639",
                "GeneratedIDs": [
                    "4219e1fd-b0ef-4c83-a8fc-302ff35f4013",
                    "109b8eeb-aa26-4601-b9e5-363b42217f0d",
                    "f0fdf750-d67a-4804-81b9-3150e9d935db"
                ]
            }
        }
    )";

  auto response = olp::parser::parse<ResponseOk>(json);

  EXPECT_STREQ(response.GetTraceID().GetParentID().c_str(),
               "2e05aaee-4c02-4735-9dbf-27eba9a47639");

  ASSERT_EQ(response.GetTraceID().GetGeneratedIDs().size(), 3);
  EXPECT_STREQ(response.GetTraceID().GetGeneratedIDs().at(0).c_str(),
               "4219e1fd-b0ef-4c83-a8fc-302ff35f4013");
  EXPECT_STREQ(response.GetTraceID().GetGeneratedIDs().at(1).c_str(),
               "109b8eeb-aa26-4601-b9e5-363b42217f0d");
  EXPECT_STREQ(response.GetTraceID().GetGeneratedIDs().at(2).c_str(),
               "f0fdf750-d67a-4804-81b9-3150e9d935db");
}

TEST(ParserTest, Catalog) {
  std::string json_input =
      "{\
    \"id\": \"roadweather-catalog-v1\",\
    \"hrn\": \"hrn:here:data:::my-catalog-v1\",\
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
      olp::parser::parse<olp::dataservice::write::model::Catalog>(json_input);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;

  ASSERT_EQ("roadweather-catalog-v1", catalog.GetId());
  ASSERT_EQ("hrn:here:data:::my-catalog-v1", catalog.GetHrn());
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
  ASSERT_EQ(24, catalog.GetLayers().at(0).GetTtl());
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

// TODO Test specfically for handling of OLP Backend bug:
// Parse dataOutThroughputMbps, dataInThroughputMbps as double even though
// OepnAPI sepcs says int64 because OLP Backend returns the value in decimal
// format (e.g. 1.0) and this triggers an assert in RapidJSON when parsing.
TEST(ParserTest, CatalogCrash) {
  std::string json_input =
      "{\"id\":\"olp-cpp-sdk-ingestion-test-catalog\",\"hrn\":\"hrn:here:data::"
      ":olp-cpp-sdk-"
      "ingestion-test-catalog\",\"name\":\"OLP CPP SDK Ingestion Test "
      "Catalog\",\"summary\":\"Test Catalog for the OLP CPP SDK Ingestion "
      "Component\",\"description\":\"Test Catalog for the OLP CPP SDK "
      "Ingestion "
      "Component.\",\"contacts\":{},\"owner\":{\"creator\":{\"id\":\"HERE-"
      "6b18d678-cde1-41fb-"
      "b1a6-9969ef253144\"},\"organisation\":{\"id\":\"olp-here-test\"}},"
      "\"tags\":[\"test\","
      "\"olp-cpp-sdk\"],\"billingTags\":[],\"created\":\"2019-02-04T22:20:24."
      "262635Z\","
      "\"replication\":{\"regions\":[{\"id\":\"eu-ireland\",\"role\":"
      "\"primary\"}]},\"layers\":"
      "[{\"id\":\"olp-cpp-sdk-ingestion-test-stream-layer\",\"hrn\":\"hrn:here:"
      "data:::olp-cpp-"
      "sdk-ingestion-test-catalog:olp-cpp-sdk-ingestion-test-stream-layer\","
      "\"name\":\"OLP CPP "
      "SDK Ingestion Test Stream Layer\",\"summary\":\"Stream Layer for OLP "
      "CPP SDK Ingestion "
      "Component Testing\",\"description\":\"Stream Layer for OLP CPP SDK "
      "Ingestion Component "
      "Testing.\",\"coverage\":{\"adminAreas\":[]},\"owner\":{\"creator\":{"
      "\"id\":\"HERE-"
      "6b18d678-cde1-41fb-b1a6-9969ef253144\"},\"organisation\":{\"id\":\"olp-"
      "here-test\"}},"
      "\"contentType\":\"text/"
      "plain\",\"ttlHours\":1,\"ttl\":600000,\"partitioningScheme\":"
      "\"generic\","
      "\"partitioning\":{\"scheme\":\"generic\"},\"volume\":{\"volumeType\":"
      "\"durable\"},"
      "\"streamProperties\":{\"dataInThroughputMbps\":1.0,"
      "\"dataOutThroughputMbps\":1.0,"
      "\"parallelization\":1},\"tags\":[\"test\",\"olp-cpp-sdk\"],"
      "\"billingTags\":[],"
      "\"created\":\"2019-02-04T23:12:35.707254Z\",\"layerType\":\"stream\"},{"
      "\"id\":\"olp-cpp-"
      "sdk-ingestion-test-stream-layer-2\",\"hrn\":\"hrn:here:data:::olp-cpp-"
      "sdk-ingestion-"
      "test-catalog:olp-cpp-sdk-ingestion-test-stream-layer-2\",\"name\":\"OLP "
      "CPP SDK "
      "Ingestion Test Stream Layer 2\",\"summary\":\"Second Stream Layer for "
      "OLP CPP SDK "
      "Ingestion Component Testing\",\"description\":\"Second Stream Layer for "
      "OLP CPP SDK "
      "Ingestion Component Testing. Content-Type differs from the "
      "first.\",\"coverage\":{\"adminAreas\":[]},\"owner\":{\"creator\":{"
      "\"id\":\"HERE-"
      "6b18d678-cde1-41fb-b1a6-9969ef253144\"},\"organisation\":{\"id\":\"olp-"
      "here-test\"}},"
      "\"contentType\":\"application/"
      "json\",\"ttlHours\":1,\"ttl\":600000,\"partitioningScheme\":\"generic\","
      "\"partitioning\":{\"scheme\":\"generic\"},\"volume\":{\"volumeType\":"
      "\"durable\"},"
      "\"streamProperties\":{\"dataInThroughputMbps\":1.0,"
      "\"dataOutThroughputMbps\":1.0,"
      "\"parallelization\":1},\"tags\":[\"test\",\"olp-cpp-sdk\"],"
      "\"billingTags\":[],"
      "\"created\":\"2019-02-05T22:11:54.412241Z\",\"layerType\":\"stream\"},{"
      "\"id\":\"olp-cpp-"
      "sdk-ingestion-test-stream-layer-sdii\",\"hrn\":\"hrn:here:data:::olp-"
      "cpp-sdk-ingestion-"
      "test-catalog:olp-cpp-sdk-ingestion-test-stream-layer-sdii\",\"name\":"
      "\"OLP CPP SDK "
      "Ingestion Test Stream Layer SDII\",\"summary\":\"SDII Stream Layer for "
      "OLP CPP SDK "
      "Ingestion Component Testing\",\"description\":\"SDII Stream Layer for "
      "OLP CPP SDK "
      "Ingestion Component "
      "Testing.\",\"coverage\":{\"adminAreas\":[]},\"owner\":{\"creator\":{"
      "\"id\":\"HERE-"
      "6b18d678-cde1-41fb-b1a6-9969ef253144\"},\"organisation\":{\"id\":\"olp-"
      "here-test\"}},"
      "\"contentType\":\"application/"
      "x-protobuf\",\"ttlHours\":1,\"ttl\":600000,\"partitioningScheme\":"
      "\"generic\","
      "\"partitioning\":{\"scheme\":\"generic\"},\"volume\":{\"volumeType\":"
      "\"durable\"},"
      "\"streamProperties\":{\"dataInThroughputMbps\":1.0,"
      "\"dataOutThroughputMbps\":1.0,"
      "\"parallelization\":1},\"tags\":[\"test\",\"olp-cpp-sdk\"],"
      "\"billingTags\":[],"
      "\"created\":\"2019-02-07T20:15:46.920639Z\",\"layerType\":\"stream\"}],"
      "\"marketplaceReady\":false,\"version\":3}";

  auto catalog =
      olp::parser::parse<olp::dataservice::write::model::Catalog>(json_input);

  ASSERT_EQ(1, catalog.GetLayers()
                   .at(1)
                   .GetStreamProperties()
                   .GetDataInThroughputMbps());
  ASSERT_EQ(1, catalog.GetLayers()
                   .at(1)
                   .GetStreamProperties()
                   .GetDataOutThroughputMbps());
}

TEST(ParserTest, Apis) {
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

  auto apis =
      olp::parser::parse<olp::dataservice::write::model::Apis>(json_input);
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

TEST(ParserTest, PublishPartition) {
  auto json = R"(
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

  auto response = olp::parser::parse<PublishPartition>(json);

  EXPECT_STREQ("314010583", response.GetPartition().get().c_str());
  EXPECT_STREQ("ff7494d6f17da702862e550c907c0a91",
               response.GetChecksum().get().c_str());
  EXPECT_EQ(152417, response.GetCompressedDataSize().get());
  EXPECT_EQ(250110, response.GetDataSize().get());
  auto data = response.GetData();
  EXPECT_STREQ(
      "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAABGdBTUEAALGPC/"
      "xhBQAAABhQTFRFvb29AACEAP8AhIKEPb5x2m9E5413aFQirhRuvAMqCw+"
      "6kE2BVsa8miQaYSKyshxFvhqdzKx8UsPYk9gDEcY1ghZXcPbENtax8g5T+"
      "3zHYufF1Lf9HdIZBfNEiKAAAAAElFTkSuQmCC",
      std::string(data->begin(), data->end()).c_str());
  EXPECT_STREQ("1b2ca68f-d4a0-4379-8120-cd025640510c",
               response.GetDataHandle().get().c_str());
  EXPECT_EQ(1519219235, response.GetTimestamp().get());
}

TEST(ParserTest, PublishPartitions) {
  auto json = R"(
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

  auto response = olp::parser::parse<PublishPartitions>(json);
  ASSERT_EQ(1, response.GetPartitions().get().size());

  auto partition = response.GetPartitions().get().at(0);

  EXPECT_STREQ("314010583", partition.GetPartition().get().c_str());
  EXPECT_STREQ("ff7494d6f17da702862e550c907c0a91",
               partition.GetChecksum().get().c_str());
  EXPECT_EQ(152417, partition.GetCompressedDataSize().get());
  EXPECT_EQ(250110, partition.GetDataSize().get());
  auto data = partition.GetData();
  EXPECT_STREQ(
      "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAABGdBTUEAALGPC/"
      "xhBQAAABhQTFRFvb29AACEAP8AhIKEPb5x2m9E5413aFQirhRuvAMqCw+"
      "6kE2BVsa8miQaYSKyshxFvhqdzKx8UsPYk9gDEcY1ghZXcPbENtax8g5T+"
      "3zHYufF1Lf9HdIZBfNEiKAAAAAElFTkSuQmCC",
      std::string(data->begin(), data->end()).c_str());
  EXPECT_STREQ("1b2ca68f-d4a0-4379-8120-cd025640510c",
               partition.GetDataHandle().get().c_str());
  EXPECT_EQ(1519219235, partition.GetTimestamp().get());
}

TEST(ParserTest, Details) {
  auto json = R"(
      {
        "state": "initialized",
        "message": "Publication initialized",
        "started": 1523459129829,
        "modified": 1523459129829,
        "expires": 1523459129829
      }
  )";

  auto response = olp::parser::parse<Details>(json);

  EXPECT_STREQ("initialized", response.GetState().c_str());
  EXPECT_STREQ("Publication initialized", response.GetMessage().c_str());
  EXPECT_EQ(1523459129829, response.GetStarted());
  EXPECT_EQ(1523459129829, response.GetModified());
  EXPECT_EQ(1523459129829, response.GetExpires());
}

TEST(ParserTest, VersionDependency) {
  auto json = R"(
      {
          "direct": true,
          "hrn": "hrn:here:data:::my-catalog",
          "version": 1
      }
)";

  auto response = olp::parser::parse<VersionDependency>(json);

  EXPECT_TRUE(response.GetDirect());
  EXPECT_STREQ("hrn:here:data:::my-catalog", response.GetHrn().c_str());
  EXPECT_EQ(1, response.GetVersion());
}

TEST(ParserTest, Publication) {
  auto json = R"(
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

  auto response = olp::parser::parse<Publication>(json);

  EXPECT_STREQ("34bc2a16-0373-4157-8ccc-19ba08a6672b",
               response.GetId().get().c_str());

  auto details = response.GetDetails().get();
  EXPECT_STREQ("initialized", details.GetState().c_str());
  EXPECT_STREQ("Publication initialized", details.GetMessage().c_str());
  EXPECT_EQ(1523459129829, details.GetStarted());
  EXPECT_EQ(1523459129829, details.GetModified());
  EXPECT_EQ(1523459129829, details.GetExpires());

  ASSERT_EQ(1, response.GetLayerIds().get().size());
  EXPECT_STREQ("my-layer", response.GetLayerIds().get().at(0).c_str());

  EXPECT_EQ(1, response.GetCatalogVersion().get());

  ASSERT_EQ(1, response.GetVersionDependencies().get().size());
  auto version_dependency = response.GetVersionDependencies().get().at(0);

  EXPECT_TRUE(version_dependency.GetDirect());
  EXPECT_STREQ("hrn:here:data:::my-catalog",
               version_dependency.GetHrn().c_str());
  EXPECT_EQ(1, version_dependency.GetVersion());
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
      olp::parser::parse<olp::dataservice::write::model::Partitions>(
          json_input);
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
      olp::parser::parse<olp::dataservice::write::model::LayerVersions>(
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

TEST(ParserTest, PublishDataRequest) {
  std::string json_input =
      R"(
      {
        "data": "payload",
        "layerId": "olp-cpp-sdk-layer",
        "traceId": "04946af8-7f0e-4d41-b85a-e883c74ebba3",
        "billingTag": "OlpCppSdkTest",
        "checksum": "olp-cpp-sdk-checksum"
      }
)";

  auto publish_data_request =
      olp::parser::parse<olp::dataservice::write::model::PublishDataRequest>(
          json_input);

  std::string data_string = "payload";
  std::shared_ptr<std::vector<unsigned char>> data_ =
      std::make_shared<std::vector<unsigned char>>(data_string.begin(),
                                                   data_string.end());

  ASSERT_EQ("OlpCppSdkTest", *publish_data_request.GetBillingTag());
  ASSERT_EQ("olp-cpp-sdk-checksum", *publish_data_request.GetChecksum());
  ASSERT_EQ(*data_, *publish_data_request.GetData());
  ASSERT_EQ("olp-cpp-sdk-layer", publish_data_request.GetLayerId());
  ASSERT_EQ("04946af8-7f0e-4d41-b85a-e883c74ebba3",
            *publish_data_request.GetTraceId());
}

}  // namespace
