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
#include <regex>
#include <string>

#include "gtest/gtest.h"

// clang-format off
// Ordering Required - Serializer template specializations before JsonSerializer.h
#include "generated/serializer/ApiSerializer.h"
#include "generated/serializer/CatalogSerializer.h"
#include "generated/serializer/LayerVersionsSerializer.h"
#include "generated/serializer/PartitionsSerializer.h"
#include "generated/serializer/VersionResponseSerializer.h"
#include "generated/serializer/JsonSerializer.h"

// clang-format on

namespace {

using namespace olp::dataservice::read::model;

void RemoveWhitespaceAndNewlines(std::string& s) {
  std::regex r("\\s+");
  s = std::regex_replace(s, r, "");
}

TEST(SerializerTest, Api) {
  std::string expectedOutput =
      "{\
      \"api\": \"config\",\
      \"version\": \"v1\",\
      \"baseURL\": \"https://config.data.api.platform.here.com/config/v1\",\
      \"parameters\": {\
      \"additionalProp1\": \"string\",\
      \"additionalProp2\": \"string\",\
      \"additionalProp3\": \"string\"\
      }\
      }";

  Api api;

  api.SetApi("config");
  api.SetVersion("v1");
  api.SetBaseUrl("https://config.data.api.platform.here.com/config/v1");
  std::map<std::string, std::string> params;
  params["additionalProp1"] = "string";
  params["additionalProp2"] = "string";
  params["additionalProp3"] = "string";
  api.SetParameters(params);

  auto startTime = std::chrono::high_resolution_clock::now();
  auto json = olp::serializer::serialize(api);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time = end - startTime;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  RemoveWhitespaceAndNewlines(expectedOutput);
  RemoveWhitespaceAndNewlines(json);
  ASSERT_EQ(expectedOutput, json);
}

TEST(SerializerTest, Catalog) {
  std::string expectedOutput =
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

  Creator creator;
  creator.SetId("string");

  Creator organisation;
  organisation.SetId("HERE");

  std::vector<std::string> admin_areas;
  admin_areas.push_back("DE");
  Coverage coverage;
  coverage.SetAdminAreas(admin_areas);

  Owner owner;
  owner.SetCreator(creator);
  owner.SetOrganisation(organisation);

  std::vector<std::string> tags;
  tags.push_back("Roads");
  tags.push_back("Weather");

  std::vector<std::string> billing_tags;
  billing_tags.push_back("Cost Center 1");
  billing_tags.push_back("Cost Center 2");

  Schema schema;
  schema.SetHrn(
      "hrn:here:schema:::com.here.schema.rib:topology-geometry_v2:2.2.0");

  Partitioning partitioning;
  std::vector<int64_t> tile_levels;
  tile_levels.push_back(12);
  partitioning.SetScheme("heretile");
  partitioning.SetTileLevels(tile_levels);

  IndexDefinition index_definition;
  index_definition.SetName("string");
  index_definition.SetType("bool");
  index_definition.SetDuration(0);
  index_definition.SetZoomLevel(0);

  std::vector<IndexDefinition> index_definitions;
  index_definitions.push_back(index_definition);

  IndexProperties index_properties;
  index_properties.SetTtl("1.year");
  index_properties.SetIndexDefinitions(index_definitions);

  StreamProperties stream_properties;
  stream_properties.SetDataInThroughputMbps(10);
  stream_properties.SetDataOutThroughputMbps(10);

  Encryption encryption;
  encryption.SetAlgorithm("aes256");

  Volume volume;
  volume.SetVolumeType("durable");
  volume.SetMaxMemoryPolicy("failOnWrite");
  volume.SetPackageType("small");
  volume.SetEncryption(encryption);

  Layer layer;
  layer.SetId("traffic-incidents");
  layer.SetName("Traffic Incidents");
  layer.SetSummary(
      "This layer provides aggregated information about traffic incidents.");
  layer.SetDescription(
      "This layer provides aggregated information about traffic incidents, "
      "including the type and location of each traffic incident, status, "
      "start and end time, and other relevant data. This data is useful to "
      "dynamically optimize route calculations.");
  layer.SetOwner(owner);
  layer.SetCoverage(coverage);
  layer.SetSchema(schema);
  layer.SetContentType("application/json");
  layer.SetContentEncoding("gzip");
  layer.SetPartitioning(partitioning);
  layer.SetLayerType("versioned");
  layer.SetDigest("SHA-1");
  layer.SetTags(tags);
  layer.SetBillingTags(billing_tags);
  layer.SetTtl(24);
  layer.SetIndexProperties(index_properties);
  layer.SetStreamProperties(stream_properties);
  layer.SetVolume(volume);

  std::vector<Layer> layers;
  layers.push_back(layer);

  Notifications notifications;
  notifications.SetEnabled(false);

  Catalog catalog;
  catalog.SetId("roadweather-catalog-v1");
  catalog.SetHrn("hrn:here:data:::my-catalog-v1");
  catalog.SetName("string");
  catalog.SetSummary(
      "Contains estimates for road conditions based on weather data.");
  catalog.SetDescription(
      "Road conditions are typically based on the temperature, comfort level, "
      "wind speed and "
      "direction. However, other weather-based data points can be taken into "
      "account.");
  catalog.SetCoverage(coverage);
  catalog.SetOwner(owner);
  catalog.SetTags(tags);
  catalog.SetBillingTags(billing_tags);
  catalog.SetCreated("2017-08-04T17:19:03.853Z");
  catalog.SetLayers(layers);
  catalog.SetVersion(1);
  catalog.SetNotifications(notifications);

  auto startTime = std::chrono::high_resolution_clock::now();
  auto json = olp::serializer::serialize(catalog);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time = end - startTime;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  RemoveWhitespaceAndNewlines(expectedOutput);
  RemoveWhitespaceAndNewlines(json);
  ASSERT_EQ(expectedOutput, json);
}

TEST(SerializerTest, LayerVersion) {
  std::string expectedOutput =
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

  LayerVersions layer_versions;

  std::vector<LayerVersion> versions;
  LayerVersion layer_version;
  layer_version.SetLayer("my-layer");
  layer_version.SetVersion(0);
  layer_version.SetTimestamp(1516397474657ll);
  versions.push_back(layer_version);

  layer_versions.SetLayerVersions(versions);
  layer_versions.SetVersion(1);

  auto startTime = std::chrono::high_resolution_clock::now();
  auto json = olp::serializer::serialize(layer_versions);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time = end - startTime;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  RemoveWhitespaceAndNewlines(expectedOutput);
  RemoveWhitespaceAndNewlines(json);
  ASSERT_EQ(expectedOutput, json);
}

TEST(SerializerTest, Partitions) {
  std::string expectedOutput =
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
    ]\
    }";

  Partition partition;
  partition.SetChecksum(
      boost::optional<std::string>("291f66029c232400e3403cd6e9cfd36e"));
  partition.SetCompressedDataSize(1024);
  partition.SetDataHandle("1b2ca68f-d4a0-4379-8120-cd025640510c");
  partition.SetDataSize(1024);
  partition.SetPartition("314010583");
  partition.SetVersion(2);

  std::vector<Partition> vector;
  vector.push_back(partition);

  Partitions partitions;
  partitions.SetPartitions(vector);

  auto startTime = std::chrono::high_resolution_clock::now();
  auto json = olp::serializer::serialize(partitions);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time = end - startTime;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  RemoveWhitespaceAndNewlines(expectedOutput);
  RemoveWhitespaceAndNewlines(json);
  ASSERT_EQ(expectedOutput, json);
}

TEST(SerializerTest, PartitionsNoCompressedDataSizeChecksumOrVersion) {
  std::string expectedOutput =
      "{\
    \"partitions\": [\
      { \
        \"dataHandle\": \"1b2ca68f-d4a0-4379-8120-cd025640510c\",\
        \"dataSize\": 1024,\
        \"partition\": \"314010583\"\
      }\
    ]\
    }";

  Partition partition;
  partition.SetDataHandle("1b2ca68f-d4a0-4379-8120-cd025640510c");
  partition.SetDataSize(1024);
  partition.SetPartition("314010583");

  std::vector<Partition> vector;
  vector.push_back(partition);

  Partitions partitions;
  partitions.SetPartitions(vector);

  auto startTime = std::chrono::high_resolution_clock::now();
  auto json = olp::serializer::serialize(partitions);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time = end - startTime;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  RemoveWhitespaceAndNewlines(expectedOutput);
  RemoveWhitespaceAndNewlines(json);
  ASSERT_EQ(expectedOutput, json);
}

TEST(SerializerTest, VersionResponse) {
  std::string expectedOutput =
      "{\
    \"version\": 0\
    }";

  VersionResponse version_response;
  version_response.SetVersion(0);

  auto startTime = std::chrono::high_resolution_clock::now();
  auto json = olp::serializer::serialize(version_response);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time = end - startTime;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  RemoveWhitespaceAndNewlines(expectedOutput);
  RemoveWhitespaceAndNewlines(json);
  ASSERT_EQ(expectedOutput, json);
}

}  // namespace
