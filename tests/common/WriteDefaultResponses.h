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

#include <random>
#include <string>
#include <utility>
#include <vector>

#include <olp/dataservice/write/generated/model/Publication.h>
#include <olp/core/generated/serializer/SerializerWrapper.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "generated/model/Api.h"

namespace mockserver {

class DefaultResponses {
 public:
  static olp::dataservice::write::model::Apis GenerateResourceApisResponse(
      std::string catalog) {
    return GenerateApisResponse({{"blob", "v1"},
                                 {"index", "v1"},
                                 {"ingest", "v1"},
                                 {"metadata", "v1"},
                                 {"notification", "v2"},
                                 {"publish", "v2"},
                                 {"query", "v1"},
                                 {"statistics", "v1"},
                                 {"stream", "v2"},
                                 {"volatile-blob", "v1"}},
                                catalog);
  }

  static olp::dataservice::write::model::Apis GeneratePlatformApisResponse() {
    return GenerateApisResponse({{"account", "v1"},
                                 {"artifact", "v1"},
                                 {"authentication", "v1"},
                                 {"authorization", "v1"},
                                 {"config", "v1"},
                                 {"consent", "v1"},
                                 {"location-service-registry", "v1"},
                                 {"lookup", "v1"},
                                 {"marketplace", "v2"},
                                 {"pipelines", "v2"}});
  }

  static olp::dataservice::write::model::Apis GenerateApisResponse(

      std::vector<std::pair<std::string, std::string>> api_types,
      std::string catalog = "") {
    olp::dataservice::write::model::Apis apis(api_types.size());
    std::string version = "v1";
    if (!catalog.empty()) {
      catalog.insert(0, "/catalogs/");
    }
    for (size_t i = 0; i < apis.size(); i++) {
      apis[i].SetApi(api_types.at(i).first);
      apis[i].SetBaseUrl("https://tmp." + api_types.at(i).first +
                         ".data.api.platform.here.com/" +
                         api_types.at(i).first + "/" + api_types.at(i).second +
                         catalog);
      apis[i].SetVersion(api_types.at(i).second);
    }
    return apis;
  }

  static olp::dataservice::write::model::Publication
  GeneratePublicationResponse(
      const std::vector<std::string>& layer_ids,
      const std::vector<olp::dataservice::write::model::VersionDependency> dependencies) {
    olp::dataservice::write::model::Publication publication;

    std::string id = "abcdefghijklmnopqrstuvwxyz0123456789-";
    std::random_device device;
    std::mt19937 generator(device());
    std::shuffle(id.begin(), id.end(), generator);
    publication.SetId(id);

    olp::dataservice::write::model::Details details;
    details.SetState("initialized");
    details.SetMessage("Publication initialized");
    details.SetStarted(1523459129829);
    details.SetModified(1523459129829);
    details.SetExpires(1523459129829);
    publication.SetDetails(details);

    if (!layer_ids.empty()) {
      publication.SetLayerIds(layer_ids);
    }

    if (!dependencies.empty()) {
      publication.SetVersionDependencies(dependencies);
    }

    publication.SetCatalogVersion(1);
    return publication;
  }
};

}  // namespace mockserver
