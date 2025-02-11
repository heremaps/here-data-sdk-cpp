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

#include <boost/json/value.hpp>

#include "CatalogSerializer.h"

#include <olp/core/generated/serializer/SerializerWrapper.h>

namespace olp {
namespace serializer {
void to_json(const dataservice::write::model::Coverage& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("adminAreas", x.GetAdminAreas(), object);
}

void to_json(const dataservice::write::model::IndexDefinition& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("name", x.GetName(), object);
  serialize("type", x.GetType(), object);
  serialize("duration", x.GetDuration(), object);
  serialize("zoomLevel", x.GetZoomLevel(), object);
}

void to_json(const dataservice::write::model::IndexProperties& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("ttl", x.GetTtl(), object);
  serialize("indexDefinitions", x.GetIndexDefinitions(), object);
}

void to_json(const dataservice::write::model::Creator& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("id", x.GetId(), object);
}

void to_json(const dataservice::write::model::Owner& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("creator", x.GetCreator(), object);
  serialize("organisation", x.GetOrganisation(), object);
}

void to_json(const dataservice::write::model::Partitioning& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("scheme", x.GetScheme(), object);
  serialize("tileLevels", x.GetTileLevels(), object);
}

void to_json(const dataservice::write::model::Schema& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("hrn", x.GetHrn(), object);
}

void to_json(const dataservice::write::model::StreamProperties& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("dataInThroughputMbps", x.GetDataInThroughputMbps(), object);
  serialize("dataOutThroughputMbps", x.GetDataOutThroughputMbps(), object);
}

void to_json(const dataservice::write::model::Encryption& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("algorithm", x.GetAlgorithm(), object);
}

void to_json(const dataservice::write::model::Volume& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("volumeType", x.GetVolumeType(), object);
  serialize("maxMemoryPolicy", x.GetMaxMemoryPolicy(), object);
  serialize("packageType", x.GetPackageType(), object);
  serialize("encryption", x.GetEncryption(), object);
}

void to_json(const dataservice::write::model::Layer& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("id", x.GetId(), object);
  serialize("name", x.GetName(), object);
  serialize("summary", x.GetSummary(), object);
  serialize("description", x.GetDescription(), object);
  serialize("owner", x.GetOwner(), object);
  serialize("coverage", x.GetCoverage(), object);
  serialize("schema", x.GetSchema(), object);
  serialize("contentType", x.GetContentType(), object);
  serialize("contentEncoding", x.GetContentEncoding(), object);
  serialize("partitioning", x.GetPartitioning(), object);
  serialize("layerType", x.GetLayerType(), object);
  serialize("digest", x.GetDigest(), object);
  serialize("tags", x.GetTags(), object);
  serialize("billingTags", x.GetBillingTags(), object);
  serialize("ttl", x.GetTtl(), object);
  serialize("indexProperties", x.GetIndexProperties(), object);
  serialize("streamProperties", x.GetStreamProperties(), object);
  serialize("volume", x.GetVolume(), object);
}

void to_json(const dataservice::write::model::Notifications& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("enabled", x.GetEnabled(), object);
}

void to_json(const dataservice::write::model::Catalog& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("id", x.GetId(), object);
  serialize("hrn", x.GetHrn(), object);
  serialize("name", x.GetName(), object);
  serialize("summary", x.GetSummary(), object);
  serialize("description", x.GetDescription(), object);
  serialize("coverage", x.GetCoverage(), object);
  serialize("owner", x.GetOwner(), object);
  serialize("tags", x.GetTags(), object);
  serialize("billingTags", x.GetBillingTags(), object);
  serialize("created", x.GetCreated(), object);
  serialize("layers", x.GetLayers(), object);
  serialize("version", x.GetVersion(), object);
  serialize("notifications", x.GetNotifications(), object);
}

}  // namespace serializer
}  // namespace olp
