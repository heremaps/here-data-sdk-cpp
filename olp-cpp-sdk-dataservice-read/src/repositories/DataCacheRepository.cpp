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

#include "DataCacheRepository.h"

#include <limits>
#include <string>

#include <olp/core/cache/KeyGenerator.h>
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
    : hrn_(hrn.ToCatalogHRNString()),
      cache_(std::move(cache)),
      default_expiry_(ConvertTime(default_expiry)) {}

client::ApiNoResponse DataCacheRepository::Put(const model::Data& data,
                                               const std::string& layer_id,
                                               const std::string& data_handle) {
  const auto key =
      cache::KeyGenerator::CreateDataHandleKey(hrn_, layer_id, data_handle);
  OLP_SDK_LOG_TRACE_F(kLogTag, "Put -> '%s'", key.c_str());

  auto write_result = cache_->Write(key, data, default_expiry_);
  if (!write_result) {
    OLP_SDK_LOG_ERROR_F(kLogTag, "Failed to write -> '%s'", key.c_str());
    return write_result.GetError();
  }

  return {client::ApiNoResult{}};
}

boost::optional<model::Data> DataCacheRepository::Get(
    const std::string& layer_id, const std::string& data_handle) {
  const auto key =
      cache::KeyGenerator::CreateDataHandleKey(hrn_, layer_id, data_handle);
  OLP_SDK_LOG_TRACE_F(kLogTag, "Get '%s'", key.c_str());

  auto cached_data = cache_->Get(key);
  if (!cached_data) {
    return boost::none;
  }

  return cached_data;
}

bool DataCacheRepository::IsCached(const std::string& layer_id,
                                   const std::string& data_handle) const {
  return cache_->Contains(
      cache::KeyGenerator::CreateDataHandleKey(hrn_, layer_id, data_handle));
}

client::ApiNoResponse DataCacheRepository::Clear(
    const std::string& layer_id, const std::string& data_handle) {
  const auto key =
      cache::KeyGenerator::CreateDataHandleKey(hrn_, layer_id, data_handle);
  OLP_SDK_LOG_TRACE_F(kLogTag, "Clear -> '%s'", key.c_str());
  return cache_->DeleteByPrefix(key);
}
void DataCacheRepository::PromoteInCache(const std::string& layer_id,
                                         const std::string& data_handle) {
  cache_->Promote(
      cache::KeyGenerator::CreateDataHandleKey(hrn_, layer_id, data_handle));
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
