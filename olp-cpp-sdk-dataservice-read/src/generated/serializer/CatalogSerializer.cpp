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

#include <rapidjson/document.h>

#include "CatalogSerializer.h"

#include <olp/core/generated/serializer/SerializerWrapper.h>

namespace olp {
namespace serializer {
void to_json(const dataservice::read::model::Coverage& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("adminAreas", x.GetAdminAreas(), value, allocator);
}

void to_json(const dataservice::read::model::IndexDefinition& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("name", x.GetName(), value, allocator);
  serialize("type", x.GetType(), value, allocator);
  serialize("duration", x.GetDuration(), value, allocator);
  serialize("zoomLevel", x.GetZoomLevel(), value, allocator);
}

void to_json(const dataservice::read::model::IndexProperties& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("ttl", x.GetTtl(), value, allocator);
  serialize("indexDefinitions", x.GetIndexDefinitions(), value, allocator);
}

void to_json(const dataservice::read::model::Creator& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("id", x.GetId(), value, allocator);
}

void to_json(const dataservice::read::model::Owner& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("creator", x.GetCreator(), value, allocator);
  serialize("organisation", x.GetOrganisation(), value, allocator);
}

void to_json(const dataservice::read::model::Partitioning& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("scheme", x.GetScheme(), value, allocator);
  serialize("tileLevels", x.GetTileLevels(), value, allocator);
}

void to_json(const dataservice::read::model::Schema& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("hrn", x.GetHrn(), value, allocator);
}

void to_json(const dataservice::read::model::StreamProperties& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("dataInThroughputMbps", x.GetDataInThroughputMbps(), value,
            allocator);
  serialize("dataOutThroughputMbps", x.GetDataOutThroughputMbps(), value,
            allocator);
}

void to_json(const dataservice::read::model::Encryption& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("algorithm", x.GetAlgorithm(), value, allocator);
}

void to_json(const dataservice::read::model::Volume& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("volumeType", x.GetVolumeType(), value, allocator);
  serialize("maxMemoryPolicy", x.GetMaxMemoryPolicy(), value, allocator);
  serialize("packageType", x.GetPackageType(), value, allocator);
  serialize("encryption", x.GetEncryption(), value, allocator);
}

void to_json(const dataservice::read::model::Layer& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("id", x.GetId(), value, allocator);
  serialize("name", x.GetName(), value, allocator);
  serialize("summary", x.GetSummary(), value, allocator);
  serialize("description", x.GetDescription(), value, allocator);
  serialize("owner", x.GetOwner(), value, allocator);
  serialize("coverage", x.GetCoverage(), value, allocator);
  serialize("schema", x.GetSchema(), value, allocator);
  serialize("contentType", x.GetContentType(), value, allocator);
  serialize("contentEncoding", x.GetContentEncoding(), value, allocator);
  serialize("partitioning", x.GetPartitioning(), value, allocator);
  serialize("layerType", x.GetLayerType(), value, allocator);
  serialize("digest", x.GetDigest(), value, allocator);
  serialize("tags", x.GetTags(), value, allocator);
  serialize("billingTags", x.GetBillingTags(), value, allocator);
  serialize("ttl", x.GetTtl(), value, allocator);
  serialize("indexProperties", x.GetIndexProperties(), value, allocator);
  serialize("streamProperties", x.GetStreamProperties(), value, allocator);
  serialize("volume", x.GetVolume(), value, allocator);
}

void to_json(const dataservice::read::model::Notifications& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("enabled", x.GetEnabled(), value, allocator);
}

void to_json(const dataservice::read::model::Catalog& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("id", x.GetId(), value, allocator);
  serialize("hrn", x.GetHrn(), value, allocator);
  serialize("name", x.GetName(), value, allocator);
  serialize("summary", x.GetSummary(), value, allocator);
  serialize("description", x.GetDescription(), value, allocator);
  serialize("coverage", x.GetCoverage(), value, allocator);
  serialize("owner", x.GetOwner(), value, allocator);
  serialize("tags", x.GetTags(), value, allocator);
  serialize("billingTags", x.GetBillingTags(), value, allocator);
  serialize("created", x.GetCreated(), value, allocator);
  serialize("layers", x.GetLayers(), value, allocator);
  serialize("version", x.GetVersion(), value, allocator);
  serialize("notifications", x.GetNotifications(), value, allocator);
}

}  // namespace serializer
}  // namespace olp
