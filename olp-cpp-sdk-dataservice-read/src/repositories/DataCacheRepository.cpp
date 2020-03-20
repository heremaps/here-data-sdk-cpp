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

#include "DataCacheRepository.h"

#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/logging/Log.h>

namespace {
constexpr auto kLogTag = "DataCacheRepository";

std::string CreateKey(const std::string& hrn, const std::string& layer_id,
                      const std::string& datahandle) {
  return hrn + "::" + layer_id + "::" + datahandle + "::Data";
}
}  // namespace

namespace olp {
namespace dataservice {
namespace read {
namespace repository {
using namespace olp::client;
DataCacheRepository::DataCacheRepository(
    const HRN& hrn, std::shared_ptr<cache::KeyValueCache> cache)
    : hrn_(hrn), cache_(cache) {}

void DataCacheRepository::Put(const model::Data& data,
                              const std::string& layer_id,
                              const std::string& data_handle) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn, layer_id, data_handle);
  OLP_SDK_LOG_TRACE_F(kLogTag, "Put '%s'", key.c_str());
  cache_->Put(key, data);
}

boost::optional<model::Data> DataCacheRepository::Get(
    const std::string& layer_id, const std::string& data_handle) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn, layer_id, data_handle);
  OLP_SDK_LOG_TRACE_F(kLogTag, "Get '%s'", key.c_str());
  auto cachedData = cache_->Get(key);
  if (!cachedData) {
    return boost::none;
  }

  return std::move(cachedData);
}

bool DataCacheRepository::Clear(const std::string& layer_id,
                                const std::string& data_handle) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn, layer_id, data_handle);
  OLP_SDK_LOG_INFO_F(kLogTag, "Clear '%s'", key.c_str());
  return cache_->RemoveKeysWithPrefix(key);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
