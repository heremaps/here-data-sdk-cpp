/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <limits>
#include <string>

#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/logging/Log.h>

namespace {
constexpr auto kLogTag = "DataCacheRepository";
constexpr auto kChronoSecondsMax = std::chrono::seconds::max();
constexpr auto kTimetMax = std::numeric_limits<time_t>::max();

time_t ConvertTime(std::chrono::seconds time) {
  return time == kChronoSecondsMax ? kTimetMax : time.count();
}
}  // namespace

namespace olp {
namespace dataservice {
namespace read {
namespace repository {
DataCacheRepository::DataCacheRepository(
    const client::HRN& hrn, std::shared_ptr<cache::KeyValueCache> cache,
    std::chrono::seconds default_expiry)
    : hrn_(hrn),
      cache_(std::move(cache)),
      default_expiry_(ConvertTime(default_expiry)) {}

client::ApiNoResponse DataCacheRepository::Put(const model::Data& data,
                                               const std::string& layer_id,
                                               const std::string& data_handle) {
  auto key = CreateKey(layer_id, data_handle);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "Put -> '%s'", key.c_str());

  if (!cache_->Put(key, data, default_expiry_)) {
    OLP_SDK_LOG_ERROR_F(kLogTag, "Failed to write -> '%s'", key.c_str());
    return {{client::ErrorCode::CacheIO, "Put to cache failed"}};
  }

  return {client::ApiNoResult{}};
}

boost::optional<model::Data> DataCacheRepository::Get(
    const std::string& layer_id, const std::string& data_handle) {
  auto key = CreateKey(layer_id, data_handle);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "Get '%s'", key.c_str());

  auto cached_data = cache_->Get(key);
  if (!cached_data) {
    return boost::none;
  }

  return cached_data;
}

bool DataCacheRepository::IsCached(const std::string& layer_id,
                                   const std::string& data_handle) const {
  auto data_key = CreateKey(layer_id, data_handle);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "IsCached key -> '%s'", data_key.c_str());
  return cache_->Contains(data_key);
}

bool DataCacheRepository::Clear(const std::string& layer_id,
                                const std::string& data_handle) {
  auto key = CreateKey(layer_id, data_handle);
  OLP_SDK_LOG_INFO_F(kLogTag, "Clear -> '%s'", key.c_str());

  return cache_->RemoveKeysWithPrefix(key);
}

std::string DataCacheRepository::CreateKey(
    const std::string& layer_id, const std::string& datahandle) const {
  std::string hrn(hrn_.ToCatalogHRNString());
  return hrn + "::" + layer_id + "::" + datahandle + "::Data";
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
