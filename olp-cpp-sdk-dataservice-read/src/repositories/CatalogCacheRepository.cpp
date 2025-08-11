/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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

#include "CatalogCacheRepository.h"

#include <string>

#include <olp/core/cache/KeyGenerator.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/logging/Log.h>

// clang-format off
#include "generated/parser/CatalogParser.h"
#include "generated/parser/VersionResponseParser.h"
#include "JsonResultParser.h"
#include "generated/serializer/CatalogSerializer.h"
#include "generated/serializer/VersionResponseSerializer.h"
#include "generated/serializer/JsonSerializer.h"
// clang-format on

namespace {
constexpr auto kLogTag = "CatalogCacheRepository";

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
CatalogCacheRepository::CatalogCacheRepository(
    const client::HRN& hrn, std::shared_ptr<cache::KeyValueCache> cache,
    std::chrono::seconds default_expiry)
    : hrn_(hrn), cache_(cache), default_expiry_(ConvertTime(default_expiry)) {}

bool CatalogCacheRepository::Put(const model::Catalog& catalog) {
  const std::string hrn(hrn_.ToCatalogHRNString());
  const auto key = cache::KeyGenerator::CreateCatalogKey(hrn);
  OLP_SDK_LOG_TRACE_F(kLogTag, "Put -> '%s'", key.c_str());

  return cache_->Put(key, catalog,
                     [&]() { return olp::serializer::serialize(catalog); },
                     default_expiry_);
}

porting::optional<model::Catalog> CatalogCacheRepository::Get() {
  const std::string hrn(hrn_.ToCatalogHRNString());
  const auto key = cache::KeyGenerator::CreateCatalogKey(hrn);
  OLP_SDK_LOG_TRACE_F(kLogTag, "Get -> '%s'", key.c_str());

  auto cached_catalog = cache_->Get(key, [](const std::string& value) {
    return parser::parse<model::Catalog>(value);
  });

  if (cached_catalog.empty()) {
    return olp::porting::none;
  }

  return boost::any_cast<model::Catalog>(cached_catalog);
}

bool CatalogCacheRepository::PutVersion(const model::VersionResponse& version) {
  const std::string hrn(hrn_.ToCatalogHRNString());
  const auto key = cache::KeyGenerator::CreateLatestVersionKey(hrn);
  OLP_SDK_LOG_TRACE_F(kLogTag, "PutVersion -> '%s'", key.c_str());

  return cache_->Put(key, version,
                     [&]() { return olp::serializer::serialize(version); },
                     default_expiry_);
}

porting::optional<model::VersionResponse> CatalogCacheRepository::GetVersion() {
  const std::string hrn(hrn_.ToCatalogHRNString());
  const auto key = cache::KeyGenerator::CreateLatestVersionKey(hrn);
  OLP_SDK_LOG_TRACE_F(kLogTag, "GetVersion -> '%s'", key.c_str());

  auto cached_version = cache_->Get(key, [](const std::string& value) {
    return parser::parse<model::VersionResponse>(value);
  });

  if (cached_version.empty()) {
    return olp::porting::none;
  }
  return boost::any_cast<model::VersionResponse>(cached_version);
}

bool CatalogCacheRepository::Clear() {
  const std::string hrn(hrn_.ToCatalogHRNString());
  const auto key = cache::KeyGenerator::CreateCatalogKey(hrn);
  OLP_SDK_LOG_TRACE_F(kLogTag, "Clear -> '%s'", key.c_str());

  return cache_->RemoveKeysWithPrefix(hrn);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
