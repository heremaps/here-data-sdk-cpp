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

std::string CreateKey(const std::string& hrn) { return hrn + "::catalog"; }
std::string VersionKey(const std::string& hrn) {
  return hrn + "::latestVersion";
}
}  // namespace

namespace olp {
namespace dataservice {
namespace read {
namespace repository {
using namespace olp::client;
CatalogCacheRepository::CatalogCacheRepository(
    const HRN& hrn, std::shared_ptr<cache::KeyValueCache> cache)
    : hrn_(hrn), cache_(cache) {}

void CatalogCacheRepository::Put(const model::Catalog& catalog) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn);
  EDGE_SDK_LOG_TRACE_F(kLogTag, "Put '%s'", key.c_str());
  cache_->Put(key, catalog,
              [catalog]() { return olp::serializer::serialize(catalog); });
}

boost::optional<model::Catalog> CatalogCacheRepository::Get() {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = CreateKey(hrn);
  EDGE_SDK_LOG_TRACE_F(kLogTag, "Get '%s'", key.c_str());
  auto cachedCatalog =
      cache_->Get(key, [](const std::string& serializedObject) {
        return parser::parse<model::Catalog>(serializedObject);
      });
  if (cachedCatalog.empty()) {
    return boost::none;
  }

  return boost::any_cast<model::Catalog>(cachedCatalog);
}

void CatalogCacheRepository::PutVersion(const model::VersionResponse& version) {
  std::string hrn(hrn_.ToCatalogHRNString());
  EDGE_SDK_LOG_TRACE_F(kLogTag, "PutVersion '%s'", hrn.c_str());
  cache_->Put(VersionKey(hrn), version,
              [version]() { return olp::serializer::serialize(version); });
}

boost::optional<model::VersionResponse> CatalogCacheRepository::GetVersion() {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto key = VersionKey(hrn);
  EDGE_SDK_LOG_TRACE_F(kLogTag, "GetVersion '%s'", key.c_str());
  auto cachedVersion =
      cache_->Get(key, [](const std::string& serializedObject) {
        return parser::parse<model::VersionResponse>(serializedObject);
      });
  if (cachedVersion.empty()) {
    return boost::none;
  }
  return boost::any_cast<model::VersionResponse>(cachedVersion);
}

void CatalogCacheRepository::Clear() {
  std::string hrn(hrn_.ToCatalogHRNString());
  EDGE_SDK_LOG_TRACE_F(kLogTag, "Clear '%s'", CreateKey(hrn).c_str());
  cache_->RemoveKeysWithPrefix(hrn);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
