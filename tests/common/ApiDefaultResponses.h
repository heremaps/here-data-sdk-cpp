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
#include <utility>
#include <vector>

#include <olp/core/client/model/Api.h>

namespace mockserver {

class ApiDefaultResponses {
 public:
  static olp::client::Apis GenerateResourceApisResponse(
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

  static olp::client::Apis GeneratePlatformApisResponse() {
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

  static olp::client::Apis GenerateApisResponse(
      std::vector<std::pair<std::string, std::string>> api_types,
      std::string catalog = "") {
    olp::client::Apis apis(api_types.size());
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
};

}  // namespace mockserver
