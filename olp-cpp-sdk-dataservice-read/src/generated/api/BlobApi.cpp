/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include "BlobApi.h"

#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>

#include <olp/core/client/OlpClient.h>

namespace olp {
namespace dataservice {
namespace read {

BlobApi::DataResponse BlobApi::GetBlob(
    const client::OlpClient& client, const std::string& layer_id,
    const model::Partition& partition, boost::optional<std::string> billing_tag,
    boost::optional<std::string> range,
    const client::CancellationContext& context) {
  std::multimap<std::string, std::string> header_params;
  header_params.emplace("Accept", "application/json");
  if (range) {
    header_params.emplace("Range", *range);
  }

  std::multimap<std::string, std::string> query_params;
  if (billing_tag) {
    query_params.emplace("billingTag", *billing_tag);
  }

  std::string metadata_uri =
      "/layers/" + layer_id + "/data/" + partition.GetDataHandle();

  std::vector<unsigned char> buffer;

  // In case we know the size in advance, we should pre-allocated a buffer.
  const auto expected_size = partition.GetDataSize();
  const auto kPartitionPreallocateLimit = 10 * 1024 * 1024;
  if (expected_size && *expected_size > 0 &&
      *expected_size < kPartitionPreallocateLimit) {
    buffer.reserve(*expected_size);
  }

  auto data_callback = [&](const std::uint8_t* data, std::uint64_t offset,
                           std::size_t length) {
    if (!offset) {
      buffer.clear();
    }

    const auto buffer_size = buffer.size();
    buffer.resize(buffer_size + length);
    std::memcpy(buffer.data() + buffer_size, data, length);
  };

  auto api_response =
      client.CallApiStream(metadata_uri, "GET", query_params, header_params,
                           data_callback, nullptr, "", context);

  if (api_response.GetStatus() != http::HttpStatusCode::OK) {
    return {client::ApiError(api_response.GetStatus()),
            api_response.GetNetworkStatistics()};
  }

  return {std::make_shared<std::vector<unsigned char>>(std::move(buffer)),
          api_response.GetNetworkStatistics()};
}
}  // namespace read
}  // namespace dataservice
}  // namespace olp
