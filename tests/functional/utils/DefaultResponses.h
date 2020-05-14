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

#include "generated/model/Api.h"
#include "olp/dataservice/read/model/Partitions.h"
#include "olp/dataservice/read/model/VersionResponse.h"

namespace mockserver {

class DefaultResponses {
 public:
  static olp::dataservice::read::model::VersionResponse GenerateVersionResponse(
      int64_t version) {
    olp::dataservice::read::model::VersionResponse version_responce;
    version_responce.SetVersion(version);
    return version_responce;
  }

  static olp::dataservice::read::model::Apis GenerateApisResponse(
      std::string catalog) {
    return GenerateApisResponse(catalog, {{"blob", "v1"},
                                          {"index", "v1"},
                                          {"ingest", "v1"},
                                          {"metadata", "v1"},
                                          {"notification", "v2"},
                                          {"publish", "v2"},
                                          {"query", "v1"},
                                          {"statistics", "v1"},
                                          {"stream", "v2"},
                                          {"volatile-blob", "v1"}});
  }

  static olp::dataservice::read::model::Apis GenerateApisResponse(
      std::string catalog,
      std::vector<std::pair<std::string, std::string>> api_types) {
    olp::dataservice::read::model::Apis apis(api_types.size());
    std::string version = "v1";
    for (size_t i = 0; i < apis.size(); i++) {
      apis[i].SetApi(api_types.at(i).first);
      apis[i].SetBaseUrl("https://tmp." + api_types.at(i).first +
                         ".data.api.platform.here.com/" +
                         api_types.at(i).first + "/" + api_types.at(i).second +
                         "/catalogs/" + catalog);
      apis[i].SetVersion(api_types.at(i).second);
    }
    return apis;
  }

  static olp::dataservice::read::model::Partitions GeneratePartitionsResponse(
      size_t size = 10) {
    std::vector<olp::dataservice::read::model::Partition> partitions_vect(size);
    for (size_t i = 0; i < partitions_vect.size(); i++) {
      partitions_vect[i].SetPartition(std::to_string(i));
      partitions_vect[i].SetDataHandle(std::to_string(i) + "-" +
                                       std::to_string(i * 100));
    }

    olp::dataservice::read::model::Partitions partitions;
    partitions.SetPartitions(partitions_vect);
    return partitions;
  }
};

}  // namespace mockserver
