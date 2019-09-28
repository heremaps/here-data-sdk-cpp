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

#include <gmock/gmock.h>

#include "CacheMock.h"

#include "../src/generated/parser/PartitionsParser.h"
#include "../src/repositories/PartitionsRepository.h"

#include "olp/core/client/OlpClientSettings.h"
#include "olp/core/generated/parser/JsonParser.h"
#include "olp/dataservice/read/DataRequest.h"

using namespace olp;
using namespace client;
using namespace dataservice::read;

TEST(PartitionsRepository, GetPartitionByIdSync) {
  using namespace testing;
  using testing::Return;

  auto cache = std::make_shared<testing::StrictMock<CacheMock>>();
  // auto network = std::make_shared<testing::StrictMock<NetworkMock>>();

  const std::string catalog = "hrn:here:data:::hereos-internal-test-v2";
  const std::string layer_id = "test_layer";
  const std::string partition_id = "269";
  const int version = 4;

  const auto catalog_hrn = HRN::FromString(catalog);

  OlpClientSettings settings;
  settings.cache = cache;
  // settings.network_request_handler = network;

  // VersionedLayerClientImplMock client(HRN::FromString(catalog), layer_id,
  //                                    settings);

  dataservice::read::DataRequest request;
  request.WithPartitionId(partition_id).WithVersion(version);

  const std::string cache_key = catalog + "::" + layer_id +
                                "::" + partition_id +
                                "::" + std::to_string(version) + "::partition";

  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] positive");

    const std::string query_cache_response =
        R"jsonString({"version":4,"partition":"269","layer":"testlayer","dataHandle":"qwerty"})jsonString";

    EXPECT_CALL(*cache, Get(cache_key, _))
        .Times(1)
        .WillOnce(
            Return(parser::parse<model::Partition>(query_cache_response)));

    client::CancellationContext context;
    auto response = repository::PartitionsRepository::GetPartitionByIdSync(
        catalog_hrn, layer_id, context, request.WithFetchOption(CacheOnly),
        settings);

    ASSERT_TRUE(response.IsSuccessful());
    const auto& result = response.GetResult();
    const auto& partitions = result.GetPartitions();
    EXPECT_EQ(partitions.size(), 1);
    EXPECT_EQ(partitions[0].GetDataHandle(), "qwerty");
    EXPECT_EQ(partitions[0].GetVersion().value_or(0), version);
    EXPECT_EQ(partitions[0].GetPartition(), partition_id);

    Mock::VerifyAndClearExpectations(cache.get());
  }
  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] negative");

    EXPECT_CALL(*cache, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(boost::any()));

    client::CancellationContext context;
    auto response = repository::PartitionsRepository::GetPartitionByIdSync(
        catalog_hrn, layer_id, context, request.WithFetchOption(CacheOnly),
        settings);

    ASSERT_FALSE(response.IsSuccessful());
    const auto& result = response.GetError();
    EXPECT_EQ(result.GetErrorCode(), ErrorCode::NotFound);

    Mock::VerifyAndClearExpectations(cache.get());
  }
  {
    SCOPED_TRACE("Fetch from cache missing partition id");

    client::CancellationContext context;
    auto response = repository::PartitionsRepository::GetPartitionByIdSync(
        catalog_hrn, layer_id, context, request.WithPartitionId(boost::none),
        settings);

    ASSERT_FALSE(response.IsSuccessful());
    const auto& result = response.GetError();
    EXPECT_EQ(result.GetErrorCode(), ErrorCode::PreconditionFailed);

    Mock::VerifyAndClearExpectations(cache.get());
  }
}