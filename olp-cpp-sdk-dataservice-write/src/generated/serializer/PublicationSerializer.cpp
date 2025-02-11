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

#include "PublicationSerializer.h"

namespace olp {
namespace serializer {
void to_json(const dataservice::write::model::Publication& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  if (x.GetId()) {
    object.emplace("id", x.GetId().get());
  }

  // TODO: Separate Details Model serializtion into it's own file when needed by
  // another model.
  if (x.GetDetails()) {
    boost::json::object details;
    details.emplace("state", x.GetDetails()->GetState());
    details.emplace("message", x.GetDetails()->GetMessage());
    details.emplace("started", x.GetDetails()->GetStarted());
    details.emplace("modified", x.GetDetails()->GetModified());
    details.emplace("expires", x.GetDetails()->GetExpires());
    object.emplace("details", std::move(details));
  }

  if (x.GetLayerIds()) {
    boost::json::array layer_ids;
    for (auto& layer_id : x.GetLayerIds().get()) {
      layer_ids.emplace_back(layer_id);
    }
    object.emplace("layerIds", std::move(layer_ids));
  }

  if (x.GetCatalogVersion()) {
    object.emplace("catalogVersion", x.GetCatalogVersion().get());
  }

  if (x.GetVersionDependencies()) {
    boost::json::array version_dependencies;
    for (auto& version_dependency : x.GetVersionDependencies().get()) {
      // TODO: Separate VersionDependency Model serializtion into it's own file
      // when needed by another model.
      boost::json::object version_dependency_json;
      version_dependency_json.emplace("direct", version_dependency.GetDirect());
      version_dependency_json.emplace("hrn", version_dependency.GetHrn());
      version_dependency_json.emplace("version",
                                      version_dependency.GetVersion());
      version_dependencies.emplace_back(std::move(version_dependency_json));
    }
    object.emplace("versionDependencies", std::move(version_dependencies));
  }
}

}  // namespace serializer

}  // namespace olp
