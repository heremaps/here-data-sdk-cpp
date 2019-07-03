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

#include "PublicationSerializer.h"

namespace olp {
namespace serializer {
void to_json(const dataservice::write::model::Publication& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  if (x.GetId()) {
    value.AddMember("id", rapidjson::StringRef(x.GetId().get().c_str()),
                    allocator);
  }

  // TODO: Separate Details Model serializtion into it's own file when needed by
  // another model.
  if (x.GetDetails()) {
    rapidjson::Value details(rapidjson::kObjectType);
    details.AddMember("state",
                      rapidjson::StringRef(x.GetDetails()->GetState().c_str()),
                      allocator);
    details.AddMember(
        "message", rapidjson::StringRef(x.GetDetails()->GetMessage().c_str()),
        allocator);
    details.AddMember("started", x.GetDetails()->GetStarted(), allocator);
    details.AddMember("modified", x.GetDetails()->GetModified(), allocator);
    details.AddMember("expires", x.GetDetails()->GetExpires(), allocator);
    value.AddMember("details", details, allocator);
  }

  if (x.GetLayerIds()) {
    rapidjson::Value layer_ids(rapidjson::kArrayType);
    for (auto& layer_id : x.GetLayerIds().get()) {
      layer_ids.PushBack(rapidjson::StringRef(layer_id.c_str()), allocator);
    }
    value.AddMember("layerIds", layer_ids, allocator);
  }

  if (x.GetCatalogVersion()) {
    value.AddMember("catalogVersion", x.GetCatalogVersion().get(), allocator);
  }

  if (x.GetVersionDependencies()) {
    rapidjson::Value version_dependencies(rapidjson::kArrayType);
    for (auto& version_dependency : x.GetVersionDependencies().get()) {
      // TODO: Separate VersionDependency Model serializtion into it's own file
      // when needed by another model.
      rapidjson::Value version_dependency_json(rapidjson::kObjectType);
      version_dependency_json.AddMember(
          "direct", version_dependency.GetDirect(), allocator);
      version_dependency_json.AddMember(
          "hrn", rapidjson::StringRef(version_dependency.GetHrn().c_str()),
          allocator);
      version_dependency_json.AddMember(
          "version", version_dependency.GetVersion(), allocator);
      version_dependencies.PushBack(version_dependency_json, allocator);
    }
    value.AddMember("versionDependencies", version_dependencies, allocator);
  }
}

}  // namespace serializer

}  // namespace olp
