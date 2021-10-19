/*
 * Copyright (C) 2021 HERE Europe B.V.
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

#include <olp/core/cache/KeyGenerator.h>

namespace {
using KeyGenerator = olp::cache::KeyGenerator;

constexpr auto kCatalogVersion = 13;
const std::string kCatalogHrn =
    "hrn:here:data::olp-here-test:hereos-internal-test-v2";
const std::string kLayerName = "some_layer";
const std::string kPartitionName = "partition";

TEST(KeyGeneratorTest, CreateApiKey) {
  {
    SCOPED_TRACE("Success");

    const std::string service_name = "random_service";
    const std::string service_version = "v8";
    const auto key =
        KeyGenerator::CreateApiKey(kCatalogHrn, service_name, service_version);

    EXPECT_EQ(key, kCatalogHrn + "::" + service_name + "::" + service_version +
                       "::api");
  }

  {
    SCOPED_TRACE("Empty values");

    // Not a special case, just make sure it will not crash.
    const auto key = KeyGenerator::CreateApiKey("", "", "");

    EXPECT_EQ(key, "::::::api");
  }
}

TEST(KeyGeneratorTest, CreateCatalogKey) {
  {
    SCOPED_TRACE("Success");

    const auto key = KeyGenerator::CreateCatalogKey(kCatalogHrn);

    EXPECT_EQ(key, kCatalogHrn + "::catalog");
  }

  {
    SCOPED_TRACE("Empty values");

    // Not a special case, just make sure it will not crash.
    const auto key = KeyGenerator::CreateCatalogKey("");

    EXPECT_EQ(key, "::catalog");
  }
}

TEST(KeyGeneratorTest, CreateLatestVersionKey) {
  {
    SCOPED_TRACE("Success");

    const auto key = KeyGenerator::CreateLatestVersionKey(kCatalogHrn);

    EXPECT_EQ(key, kCatalogHrn + "::latestVersion");
  }

  {
    SCOPED_TRACE("Empty values");

    // Not a special case, just make sure it will not crash.
    const auto key = KeyGenerator::CreateLatestVersionKey("");

    EXPECT_EQ(key, "::latestVersion");
  }
}

TEST(KeyGeneratorTest, CreatePartitionKey) {
  {
    SCOPED_TRACE("Success");

    const auto key = KeyGenerator::CreatePartitionKey(
        kCatalogHrn, kLayerName, kPartitionName, kCatalogVersion);

    EXPECT_EQ(key, kCatalogHrn + "::" + kLayerName + "::" + kPartitionName +
                       "::" + std::to_string(kCatalogVersion) + "::partition");
  }

  {
    SCOPED_TRACE("No version");

    const auto key = KeyGenerator::CreatePartitionKey(
        kCatalogHrn, kLayerName, kPartitionName, boost::none);

    EXPECT_EQ(key, kCatalogHrn + "::" + kLayerName + "::" + kPartitionName +
                       "::partition");
  }

  {
    SCOPED_TRACE("Empty values");

    // Not a special case, just make sure it will not crash.
    const auto key = KeyGenerator::CreatePartitionKey("", "", "", boost::none);

    EXPECT_EQ(key, "::::::partition");
  }
}

TEST(KeyGeneratorTest, CreatePatitionsKey) {
  {
    SCOPED_TRACE("Success");

    const auto key = KeyGenerator::CreatePartitionsKey(kCatalogHrn, kLayerName,
                                                       kCatalogVersion);

    EXPECT_EQ(key, kCatalogHrn + "::" + kLayerName +
                       "::" + std::to_string(kCatalogVersion) + "::partitions");
  }

  {
    SCOPED_TRACE("No version");

    const auto key =
        KeyGenerator::CreatePartitionsKey(kCatalogHrn, kLayerName, boost::none);

    EXPECT_EQ(key, kCatalogHrn + "::" + kLayerName + "::partitions");
  }

  {
    SCOPED_TRACE("Empty values");

    // Not a special case, just make sure it will not crash.
    const auto key = KeyGenerator::CreatePartitionsKey("", "", boost::none);

    EXPECT_EQ(key, "::::partitions");
  }
}

TEST(KeyGeneratorTest, CreateLayerVersionsKey) {
  {
    SCOPED_TRACE("Success");

    const auto key =
        KeyGenerator::CreateLayerVersionsKey(kCatalogHrn, kCatalogVersion);

    EXPECT_EQ(key, kCatalogHrn + "::" + std::to_string(kCatalogVersion) +
                       "::layerVersions");
  }

  {
    SCOPED_TRACE("Empty values");

    // Not a special case, just make sure it will not crash.
    const auto key = KeyGenerator::CreateLayerVersionsKey("", kCatalogVersion);

    EXPECT_EQ(key, "::" + std::to_string(kCatalogVersion) + "::layerVersions");
  }
}

TEST(KeyGeneratorTest, CreateQuadTreeKey) {
  const auto root_tile = olp::geo::TileKey::FromRowColumnLevel(0, 0, 0);
  const auto depth = 4;

  {
    SCOPED_TRACE("Success");

    const auto key = KeyGenerator::CreateQuadTreeKey(
        kCatalogHrn, kLayerName, root_tile, kCatalogVersion, depth);

    EXPECT_EQ(key, kCatalogHrn + "::" + kLayerName +
                       "::" + root_tile.ToHereTile() +
                       "::" + std::to_string(kCatalogVersion) +
                       "::" + std::to_string(depth) + "::quadtree");
  }

  {
    SCOPED_TRACE("No version");

    const auto key = KeyGenerator::CreateQuadTreeKey(
        kCatalogHrn, kLayerName, root_tile, boost::none, depth);

    EXPECT_EQ(key, kCatalogHrn + "::" + kLayerName +
                       "::" + root_tile.ToHereTile() +
                       "::" + std::to_string(depth) + "::quadtree");
  }

  {
    SCOPED_TRACE("Empty values");

    // Not a special case, just make sure it will not crash.
    const auto key =
        KeyGenerator::CreateQuadTreeKey("", "", root_tile, boost::none, depth);

    EXPECT_EQ(key, "::::" + root_tile.ToHereTile() +
                       "::" + std::to_string(depth) + "::quadtree");
  }
}

TEST(KeyGeneratorTest, CreateDataHandleKey) {
  {
    SCOPED_TRACE("Success");

    const auto data_handle = "data_handle";
    const auto key =
        KeyGenerator::CreateDataHandleKey(kCatalogHrn, kLayerName, data_handle);

    EXPECT_EQ(key,
              kCatalogHrn + "::" + kLayerName + "::" + data_handle + "::Data");
  }

  {
    SCOPED_TRACE("Empty values");

    // Not a special case, just make sure it will not crash.
    const auto key = KeyGenerator::CreateDataHandleKey("", "", "");

    EXPECT_EQ(key, "::::::Data");
  }
}

}  // namespace
