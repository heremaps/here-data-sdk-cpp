/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include "CatalogSettings.h"

#include <olp/core/cache/CacheSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <boost/format.hpp>
#include "ApiClientLookup.h"
#include "generated/ConfigApi.h"

// clang-format off
#include <generated/serializer/CatalogSerializer.h>
#include <generated/serializer/JsonSerializer.h>
#include <generated/parser/CatalogParser.h>
#include <olp/core/generated/parser/JsonParser.h>
// clang-format on

namespace olp {
namespace dataservice {
namespace write {

CatalogSettings::CatalogSettings(const client::HRN catalog,
                                 client::OlpClientSettings settings)
    : catalog_(std::move(catalog)),
      cache_(settings.cache),
      settings_(std::move(settings)) {
  if (!settings_.cache) {
    settings_.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});
    cache_ = settings_.cache;
  }
}

CatalogSettings::LayerSettingsResult CatalogSettings::GetLayerSettingsFromModel(
    const model::Catalog& catalog, const std::string& layer_id) const {
  const auto& layers = catalog.GetLayers();

  auto layer_it = std::find_if(
      layers.begin(), layers.end(),
      [&](const model::Layer& layer) { return layer.GetId() == layer_id; });

  CatalogSettings::LayerSettings settings;
  if (layer_it != layers.end()) {
    return CatalogSettings::LayerSettings{layer_it->GetContentType(),
                                          layer_it->GetContentEncoding()};
  }

  return client::ApiError(
      client::ErrorCode::InvalidArgument,
      (boost::format("Layer '%1%' not found in catalog '%2%'") % layer_id %
       catalog_.ToString())
          .str());
}

CatalogSettings::LayerSettingsResult CatalogSettings::GetLayerSettings(
    client::CancellationContext context, BillingTag billing_tag,
    const std::string& layer_id) const {
  const auto catalog_settings_key = catalog_.ToString() + "::catalog";

  if (!cache_->Contains(catalog_settings_key)) {
    auto lookup_response = ApiClientLookup::LookupApiClient(
        catalog_, context, "config", "v1", settings_);
    if (!lookup_response.IsSuccessful()) {
      return lookup_response.GetError();
    }

    client::OlpClient client = lookup_response.GetResult();
    auto catalog_response = ConfigApi::GetCatalog(client, catalog_.ToString(),
                                                  billing_tag, context);

    if (!catalog_response.IsSuccessful()) {
      return catalog_response.GetError();
    }

    auto catalog_model = catalog_response.MoveResult();

    cache_->Put(
        catalog_settings_key, catalog_model,
        [&]() { return serializer::serialize<model::Catalog>(catalog_model); },
        settings_.default_cache_expiration.count());

    return GetLayerSettingsFromModel(catalog_model, layer_id);
  }

  LayerSettings settings;

  const auto& cached_catalog =
      cache_->Get(catalog_settings_key, [](const std::string& model) {
        return parser::parse<model::Catalog>(model);
      });

  if (cached_catalog.empty()) {
    return client::ApiError(
        client::ErrorCode::Unknown,
        (boost::format("Cached catalog '%1' is empty") % catalog_.ToString())
            .str());
  }

  const auto& catalog =
      olp::porting::any_cast<const model::Catalog&>(cached_catalog);
  return GetLayerSettingsFromModel(catalog, layer_id);
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
