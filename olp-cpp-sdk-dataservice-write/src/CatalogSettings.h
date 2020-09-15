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

#pragma once

#include <string>

#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>

namespace olp {
namespace dataservice {
namespace write {

namespace model {
class Catalog;
}

class CatalogSettings {
 public:
  using BillingTag = boost::optional<std::string>;

  struct LayerSettings {
    std::string content_type;
    std::string content_encoding;
  };
  using LayerSettingsResult =
      client::ApiResponse<LayerSettings, client::ApiError>;

  CatalogSettings(const client::HRN catalog,
                  client::OlpClientSettings settings);

  LayerSettingsResult GetLayerSettings(client::CancellationContext context,
                                       BillingTag billing_tag,
                                       const std::string& layer_id) const;

 private:
  LayerSettingsResult GetLayerSettingsFromModel(
      const model::Catalog& catalog, const std::string& layer_id) const;

  client::HRN catalog_;
  std::shared_ptr<cache::KeyValueCache> cache_;
  client::OlpClientSettings settings_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
