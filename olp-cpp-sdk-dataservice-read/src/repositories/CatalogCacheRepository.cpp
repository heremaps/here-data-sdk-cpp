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

#include "CatalogCacheRepository.h"

#include <string>

#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/logging/Log.h>

// clang-format off
#include "generated/parser/CatalogParser.h"
#include "generated/parser/VersionResponseParser.h"
#include <olp/core/generated/parser/JsonParser.h>
#include "generated/serializer/CatalogSerializer.h"
#include "generated/serializer/VersionResponseSerializer.h"
#include "generated/serializer/JsonSerializer.h"
// clang-format on

namespace {
constexpr auto kLogTag = "CatalogCacheRepository";

// Currently, we expire the catalog version after 5 minutes. Later we plan to
// give the user the control when to expire it.
constexpr auto kCatalogVersionExpiryTime = 5 * 60;
constexpr auto kChronoSecondsMax = std::chrono::seconds::max();
constexpr auto kTimetMax = std::numeric_limits<time_t>::max();

std::string CreateKey(const std::string& hrn) { return hrn + "::catalog"; }
std::string VersionKey(const std::string& hrn) {
  return hrn + "::latestVersion";
}

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

void CatalogCacheRepository::Put(const model::Catalog& catalog) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "Put -> '%s'", key.c_str());

  cache_->Put(key, catalog,
              [&]() { return olp::serializer::serialize(catalog); },
              default_expiry_);
}

boost::optional<model::Catalog> CatalogCacheRepository::Get() {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "Get -> '%s'", key.c_str());

  auto cached_catalog = cache_->Get(key, [](const std::string& value) {
    return parser::parse<model::Catalog>(value);
  });

  if (cached_catalog.empty()) {
    return boost::none;
  }

  return boost::any_cast<model::Catalog>(cached_catalog);
}

void CatalogCacheRepository::PutVersion(const model::VersionResponse& version) {
  std::string hrn(hrn_.ToCatalogHRNString());
  OLP_SDK_LOG_DEBUG_F(kLogTag, "PutVersion -> '%s'", hrn.c_str());

  cache_->Put(VersionKey(hrn), version,
              [&]() { return olp::serializer::serialize(version); },
              kCatalogVersionExpiryTime);
}

boost::optional<model::VersionResponse> CatalogCacheRepository::GetVersion() {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = VersionKey(hrn);
  OLP_SDK_LOG_DEBUG_F(kLogTag, "GetVersion -> '%s'", key.c_str());

  auto cached_version = cache_->Get(key, [](const std::string& value) {
    return parser::parse<model::VersionResponse>(value);
  });

  if (cached_version.empty()) {
    return boost::none;
  }
  return boost::any_cast<model::VersionResponse>(cached_version);
}

void CatalogCacheRepository::Clear() {
  std::string hrn(hrn_.ToCatalogHRNString());
  OLP_SDK_LOG_INFO_F(kLogTag, "Clear -> '%s'", CreateKey(hrn).c_str());

  cache_->RemoveKeysWithPrefix(hrn);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
