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

#include "CatalogParser.h"

#include <olp/core/generated/parser/ParserWrapper.h>

namespace olp {
namespace parser {
namespace model = dataservice::read::model;

void from_json(const rapidjson::Value& value, model::Coverage& x) {
  x.SetAdminAreas(parse<std::vector<std::string>>(value, "adminAreas"));
}

void from_json(const rapidjson::Value& value, model::IndexDefinition& x) {
  x.SetName(parse<std::string>(value, "name"));
  x.SetType(parse<std::string>(value, "type"));
  x.SetDuration(parse<int64_t>(value, "duration"));
  x.SetZoomLevel(parse<int64_t>(value, "zoomLevel"));
}

void from_json(const rapidjson::Value& value, model::IndexProperties& x) {
  x.SetTtl(parse<std::string>(value, "ttl"));
  x.SetIndexDefinitions(
      parse<std::vector<model::IndexDefinition>>(value, "indexDefinitions"));
}

void from_json(const rapidjson::Value& value, model::Creator& x) {
  x.SetId(parse<std::string>(value, "id"));
}

void from_json(const rapidjson::Value& value, model::Owner& x) {
  x.SetCreator(parse<model::Creator>(value, "creator"));
  x.SetOrganisation(parse<model::Creator>(value, "organisation"));
}

void from_json(const rapidjson::Value& value, model::Partitioning& x) {
  x.SetScheme(parse<std::string>(value, "scheme"));
  x.SetTileLevels(parse<std::vector<int64_t>>(value, "tileLevels"));
}

void from_json(const rapidjson::Value& value, model::Schema& x) {
  x.SetHrn(parse<std::string>(value, "hrn"));
}

void from_json(const rapidjson::Value& value, model::StreamProperties& x) {
  // Parsing these as double even though OepnAPI sepcs says int64 because
  // Backend returns the value in decimal format (e.g. 1.0) and this triggers an
  // assert in RapidJSON when parsing.
  x.SetDataInThroughputMbps(
      static_cast<int64_t>(parse<double>(value, "dataInThroughputMbps")));
  x.SetDataOutThroughputMbps(
      static_cast<int64_t>(parse<double>(value, "dataOutThroughputMbps")));
}

void from_json(const rapidjson::Value& value, model::Encryption& x) {
  x.SetAlgorithm(parse<std::string>(value, "algorithm"));
}

void from_json(const rapidjson::Value& value, model::Volume& x) {
  x.SetVolumeType(parse<std::string>(value, "volumeType"));
  x.SetMaxMemoryPolicy(parse<std::string>(value, "maxMemoryPolicy"));
  x.SetPackageType(parse<std::string>(value, "packageType"));
  x.SetEncryption(parse<model::Encryption>(value, "encryption"));
}

void from_json(const rapidjson::Value& value, model::Layer& x) {
  x.SetId(parse<std::string>(value, "id"));
  x.SetName(parse<std::string>(value, "name"));
  x.SetSummary(parse<std::string>(value, "summary"));
  x.SetDescription(parse<std::string>(value, "description"));
  x.SetOwner(parse<model::Owner>(value, "owner"));
  x.SetCoverage(parse<model::Coverage>(value, "coverage"));
  x.SetSchema(parse<model::Schema>(value, "schema"));
  x.SetContentType(parse<std::string>(value, "contentType"));
  x.SetContentEncoding(parse<std::string>(value, "contentEncoding"));
  x.SetPartitioning(parse<model::Partitioning>(value, "partitioning"));
  x.SetLayerType(parse<std::string>(value, "layerType"));
  x.SetDigest(parse<std::string>(value, "digest"));
  x.SetTags(parse<std::vector<std::string>>(value, "tags"));
  x.SetBillingTags(parse<std::vector<std::string>>(value, "billingTags"));
  x.SetTtl(parse<porting::optional<int64_t>>(value, "ttl"));
  x.SetIndexProperties(parse<model::IndexProperties>(value, "indexProperties"));
  x.SetStreamProperties(
      parse<model::StreamProperties>(value, "streamProperties"));
  x.SetVolume(parse<model::Volume>(value, "volume"));
}

void from_json(const rapidjson::Value& value, model::Notifications& x) {
  x.SetEnabled(parse<bool>(value, "enabled"));
}

void from_json(const rapidjson::Value& value, model::Catalog& x) {
  x.SetId(parse<std::string>(value, "id"));
  x.SetHrn(parse<std::string>(value, "hrn"));
  x.SetName(parse<std::string>(value, "name"));
  x.SetSummary(parse<std::string>(value, "summary"));
  x.SetDescription(parse<std::string>(value, "description"));
  x.SetCoverage(parse<model::Coverage>(value, "coverage"));
  x.SetOwner(parse<model::Owner>(value, "owner"));
  x.SetTags(parse<std::vector<std::string>>(value, "tags"));
  x.SetBillingTags(parse<std::vector<std::string>>(value, "billingTags"));
  x.SetCreated(parse<std::string>(value, "created"));
  x.SetLayers(parse<std::vector<model::Layer>>(value, "layers"));
  x.SetVersion(parse<int64_t>(value, "version"));
  x.SetNotifications(parse<model::Notifications>(value, "notifications"));
}

}  // namespace parser

}  // namespace olp
