/*
 * Copyright (C) 2019-2023 HERE Europe B.V.
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
#include <vector>

namespace olp {
namespace dataservice {
namespace write {
namespace model {
/**
 * @brief The areas the catalog describes.
 */
class Coverage {
 public:
  Coverage() = default;
  Coverage(const Coverage&) = default;
  Coverage(Coverage&&) = default;
  Coverage& operator=(const Coverage&) = default;
  Coverage& operator=(Coverage&&) = default;
  virtual ~Coverage() = default;

 private:
  std::vector<std::string> admin_areas_;

 public:
  const std::vector<std::string>& GetAdminAreas() const { return admin_areas_; }
  std::vector<std::string>& GetMutableAdminAreas() { return admin_areas_; }
  void SetAdminAreas(const std::vector<std::string>& value) {
    this->admin_areas_ = value;
  }
};

/**
 * @brief Model to represent the definition of index.
 */
class IndexDefinition {
 public:
  IndexDefinition() = default;
  IndexDefinition(const IndexDefinition&) = default;
  IndexDefinition(IndexDefinition&&) = default;
  IndexDefinition& operator=(const IndexDefinition&) = default;
  IndexDefinition& operator=(IndexDefinition&&) = default;
  virtual ~IndexDefinition() = default;

 private:
  std::string name_;
  std::string type_;
  int64_t duration_{0};
  int64_t zoomLevel_{0};

 public:
  const std::string& GetName() const { return name_; }
  std::string& GetMutableName() { return name_; }
  void SetName(const std::string& value) { this->name_ = value; }

  const std::string& GetType() const { return type_; }
  std::string& GetMutableType() { return type_; }
  void SetType(const std::string& value) { this->type_ = value; }

  const int64_t& GetDuration() const { return duration_; }
  int64_t& GetMutableDuration() { return duration_; }
  void SetDuration(const int64_t& value) { this->duration_ = value; }

  const int64_t& GetZoomLevel() const { return zoomLevel_; }
  int64_t& GetMutableZoomLevel() { return zoomLevel_; }
  void SetZoomLevel(const int64_t& value) { this->zoomLevel_ = value; }
};

/**
 * @brief Model to index properties.
 */
class IndexProperties {
 public:
  IndexProperties() = default;
  IndexProperties(const IndexProperties&) = default;
  IndexProperties(IndexProperties&&) = default;
  IndexProperties& operator=(const IndexProperties&) = default;
  IndexProperties& operator=(IndexProperties&&) = default;
  virtual ~IndexProperties() = default;

 private:
  std::string ttl_;
  std::vector<IndexDefinition> index_definitions_;

 public:
  const std::string& GetTtl() const { return ttl_; }
  std::string& GetMutableTtl() { return ttl_; }
  void SetTtl(const std::string& value) { this->ttl_ = value; }

  const std::vector<IndexDefinition>& GetIndexDefinitions() const {
    return index_definitions_;
  }
  std::vector<IndexDefinition>& GetMutableIndexDefinitions() {
    return index_definitions_;
  }
  void SetIndexDefinitions(const std::vector<IndexDefinition>& value) {
    this->index_definitions_ = value;
  }
};

/**
 * @brief Creator of the catalog.
 */
class Creator {
 public:
  Creator() = default;
  Creator(const Creator&) = default;
  Creator(Creator&&) = default;
  Creator& operator=(const Creator&) = default;
  Creator& operator=(Creator&&) = default;
  virtual ~Creator() = default;

 private:
  std::string id_;

 public:
  const std::string& GetId() const { return id_; }
  std::string& GetMutableId() { return id_; }
  void SetId(const std::string& value) { this->id_ = value; }
};

/**
 * @brief Owner of the catalog.
 */
class Owner {
 public:
  Owner() = default;
  Owner(const Owner&) = default;
  Owner(Owner&&) = default;
  Owner& operator=(const Owner&) = default;
  Owner& operator=(Owner&&) = default;
  virtual ~Owner() = default;

 private:
  Creator creator_;
  Creator organisation_;

 public:
  const Creator& GetCreator() const { return creator_; }
  Creator& GetMutableCreator() { return creator_; }
  void SetCreator(const Creator& value) { this->creator_ = value; }

  const Creator& GetOrganisation() const { return organisation_; }
  Creator& GetMutableOrganisation() { return organisation_; }
  void SetOrganisation(const Creator& value) { this->organisation_ = value; }
};

/**
 * @brief The paritioning scheme of the catalog.
 */
class Partitioning {
 public:
  Partitioning() = default;
  Partitioning(const Partitioning&) = default;
  Partitioning(Partitioning&&) = default;
  Partitioning& operator=(const Partitioning&) = default;
  Partitioning& operator=(Partitioning&&) = default;
  virtual ~Partitioning() = default;

 private:
  std::string scheme_;
  std::vector<int64_t> tile_levels_;

 public:
  const std::string& GetScheme() const { return scheme_; }
  std::string& GetMutableScheme() { return scheme_; }
  void SetScheme(const std::string& value) { this->scheme_ = value; }

  const std::vector<int64_t>& GetTileLevels() const { return tile_levels_; }
  std::vector<int64_t>& GetMutableTileLevels() { return tile_levels_; }
  void SetTileLevels(const std::vector<int64_t>& value) {
    this->tile_levels_ = value;
  }
};

/**
 * @brief The schema of the catalog.
 */
class Schema {
 public:
  Schema() = default;
  Schema(const Schema&) = default;
  Schema(Schema&&) = default;
  Schema& operator=(const Schema&) = default;
  Schema& operator=(Schema&&) = default;
  virtual ~Schema() = default;

 private:
  std::string hrn_;

 public:
  const std::string& GetHrn() const { return hrn_; }
  std::string& GetMutableHrn() { return hrn_; }
  void SetHrn(const std::string& value) { this->hrn_ = value; }
};

/**
 * @brief The stream properties for the layer of the catalog.
 */
class StreamProperties {
 public:
  StreamProperties() = default;
  StreamProperties(const StreamProperties&) = default;
  StreamProperties(StreamProperties&&) = default;
  StreamProperties& operator=(const StreamProperties&) = default;
  StreamProperties& operator=(StreamProperties&&) = default;
  virtual ~StreamProperties() = default;

 private:
  int64_t data_in_throughput_mbps_{0};
  int64_t data_out_throughput_mbps_{0};

 public:
  const int64_t& GetDataInThroughputMbps() const {
    return data_in_throughput_mbps_;
  }
  int64_t& GetMutableDataInThroughputMbps() { return data_in_throughput_mbps_; }
  void SetDataInThroughputMbps(const int64_t& value) {
    this->data_in_throughput_mbps_ = value;
  }

  const int64_t& GetDataOutThroughputMbps() const {
    return data_out_throughput_mbps_;
  }
  int64_t& GetMutableDataOutThroughputMbps() {
    return data_out_throughput_mbps_;
  }
  void SetDataOutThroughputMbps(const int64_t& value) {
    this->data_out_throughput_mbps_ = value;
  }
};

/**
 * @brief The encryption scheme the catalog.
 */
class Encryption {
 public:
  Encryption() = default;
  Encryption(const Encryption&) = default;
  Encryption(Encryption&&) = default;
  Encryption& operator=(const Encryption&) = default;
  Encryption& operator=(Encryption&&) = default;
  virtual ~Encryption() = default;

 private:
  std::string algorithm_;

 public:
  const std::string& GetAlgorithm() const { return algorithm_; }
  std::string& GetMutableAlgorithm() { return algorithm_; }
  void SetAlgorithm(const std::string& value) { this->algorithm_ = value; }
};

/**
 * @brief The storage details of a catalog layer.
 */
class Volume {
 public:
  Volume() = default;
  Volume(const Volume&) = default;
  Volume(Volume&&) = default;
  Volume& operator=(const Volume&) = default;
  Volume& operator=(Volume&&) = default;
  virtual ~Volume() = default;

 private:
  std::string volume_type_;
  std::string max_memory_policy_;
  std::string package_type_;
  Encryption encryption_;

 public:
  const std::string& GetVolumeType() const { return volume_type_; }
  std::string& GetMutableVolumeType() { return volume_type_; }
  void SetVolumeType(const std::string& value) { this->volume_type_ = value; }

  const std::string& GetMaxMemoryPolicy() const { return max_memory_policy_; }
  std::string& GetMutableMaxMemoryPolicy() { return max_memory_policy_; }
  void SetMaxMemoryPolicy(const std::string& value) {
    this->max_memory_policy_ = value;
  }

  const std::string& GetPackageType() const { return package_type_; }
  std::string& GetMutablePackageType() { return package_type_; }
  void SetPackageType(const std::string& value) { this->package_type_ = value; }

  const Encryption& GetEncryption() const { return encryption_; }
  Encryption& GetMutableEncryption() { return encryption_; }
  void SetEncryption(const Encryption& value) { this->encryption_ = value; }
};

/**
 * @brief A layer of the catalog.
 */
class Layer {
 public:
  Layer() = default;
  Layer(const Layer&) = default;
  Layer(Layer&&) = default;
  Layer& operator=(const Layer&) = default;
  Layer& operator=(Layer&&) = default;
  virtual ~Layer() = default;

 private:
  std::string id_;
  std::string name_;
  std::string summary_;
  std::string description_;
  Owner owner_;
  Coverage coverage_;
  Schema schema_;
  std::string content_type_;
  std::string content_encoding_;
  Partitioning partitioning_;
  std::string layer_type_;
  std::string digest_;
  std::vector<std::string> tags_;
  std::vector<std::string> billing_tags_;
  int64_t ttl_{0};
  IndexProperties index_properties_;
  StreamProperties stream_properties_;
  Volume volume_;

 public:
  const std::string& GetId() const { return id_; }
  std::string& GetMutableId() { return id_; }
  void SetId(const std::string& value) { this->id_ = value; }

  const std::string& GetName() const { return name_; }
  std::string& GetMutableName() { return name_; }
  void SetName(const std::string& value) { this->name_ = value; }

  const std::string& GetSummary() const { return summary_; }
  std::string& GetMutableSummary() { return summary_; }
  void SetSummary(const std::string& value) { this->summary_ = value; }

  const std::string& GetDescription() const { return description_; }
  std::string& GetMutableDescription() { return description_; }
  void SetDescription(const std::string& value) { this->description_ = value; }

  const Owner& GetOwner() const { return owner_; }
  Owner& GetMutableOwner() { return owner_; }
  void SetOwner(const Owner& value) { this->owner_ = value; }

  const Coverage& GetCoverage() const { return coverage_; }
  Coverage& GetMutableCoverage() { return coverage_; }
  void SetCoverage(const Coverage& value) { this->coverage_ = value; }

  const Schema& GetSchema() const { return schema_; }
  Schema& GetMutableSchema() { return schema_; }
  void SetSchema(const Schema& value) { this->schema_ = value; }

  const std::string& GetContentType() const { return content_type_; }
  std::string& GetMutableContentType() { return content_type_; }
  void SetContentType(const std::string& value) { this->content_type_ = value; }

  const std::string& GetContentEncoding() const { return content_encoding_; }
  std::string& GetMutableContentEncoding() { return content_encoding_; }
  void SetContentEncoding(const std::string& value) {
    this->content_encoding_ = value;
  }

  const Partitioning& GetPartitioning() const { return partitioning_; }
  Partitioning& GetMutablePartitioning() { return partitioning_; }
  void SetPartitioning(const Partitioning& value) {
    this->partitioning_ = value;
  }

  const std::string& GetLayerType() const { return layer_type_; }
  std::string& GetMutableLayerType() { return layer_type_; }
  void SetLayerType(const std::string& value) { this->layer_type_ = value; }

  const std::string& GetDigest() const { return digest_; }
  std::string& GetMutableDigest() { return digest_; }
  void SetDigest(const std::string& value) { this->digest_ = value; }

  const std::vector<std::string>& GetTags() const { return tags_; }
  std::vector<std::string>& GetMutableTags() { return tags_; }
  void SetTags(const std::vector<std::string>& value) { this->tags_ = value; }

  const std::vector<std::string>& GetBillingTags() const {
    return billing_tags_;
  }
  std::vector<std::string>& GetMutableBillingTags() { return billing_tags_; }
  void SetBillingTags(const std::vector<std::string>& value) {
    this->billing_tags_ = value;
  }

  const int64_t& GetTtl() const { return ttl_; }
  int64_t& GetMutableTtl() { return ttl_; }
  void SetTtl(const int64_t& value) { this->ttl_ = value; }

  const IndexProperties& GetIndexProperties() const {
    return index_properties_;
  }
  IndexProperties& GetMutableIndexProperties() { return index_properties_; }
  void SetIndexProperties(const IndexProperties& value) {
    this->index_properties_ = value;
  }

  const StreamProperties& GetStreamProperties() const {
    return stream_properties_;
  }
  StreamProperties& GetMutableStreamProperties() { return stream_properties_; }
  void SetStreamProperties(const StreamProperties& value) {
    this->stream_properties_ = value;
  }

  const Volume& GetVolume() const { return volume_; }
  Volume& GetMutableVolume() { return volume_; }
  void SetVolume(const Volume& value) { this->volume_ = value; }
};

/**
 * @brief Catalog notifications.
 */
class Notifications {
 public:
  Notifications() = default;
  Notifications(const Notifications&) = default;
  Notifications(Notifications&&) = default;
  Notifications& operator=(const Notifications&) = default;
  Notifications& operator=(Notifications&&) = default;
  virtual ~Notifications() = default;

 private:
  bool enabled_{false};

 public:
  const bool& GetEnabled() const { return enabled_; }
  bool& GetMutableEnabled() { return enabled_; }
  void SetEnabled(const bool& value) { this->enabled_ = value; }
};

/**
 * @brief Model to represent catalog.
 */
class Catalog {
 public:
  Catalog() = default;
  Catalog(const Catalog&) = default;
  Catalog(Catalog&&) = default;
  Catalog& operator=(const Catalog&) = default;
  Catalog& operator=(Catalog&&) = default;
  virtual ~Catalog() = default;

 private:
  std::string id_;
  std::string hrn_;
  std::string name_;
  std::string summary_;
  std::string description_;
  Coverage coverage_;
  Owner owner_;
  std::vector<std::string> tags_;
  std::vector<std::string> billing_tags_;
  std::string created_;
  std::vector<Layer> layers_;
  int64_t version_{0};
  Notifications notifications_;

 public:
  const std::string& GetId() const { return id_; }
  std::string& GetMutableId() { return id_; }
  void SetId(const std::string& value) { this->id_ = value; }

  const std::string& GetHrn() const { return hrn_; }
  std::string& GetMutableHrn() { return hrn_; }
  void SetHrn(const std::string& value) { this->hrn_ = value; }

  const std::string& GetName() const { return name_; }
  std::string& GetMutableName() { return name_; }
  void SetName(const std::string& value) { this->name_ = value; }

  const std::string& GetSummary() const { return summary_; }
  std::string& GetMutableSummary() { return summary_; }
  void SetSummary(const std::string& value) { this->summary_ = value; }

  const std::string& GetDescription() const { return description_; }
  std::string& GetMutableDescription() { return description_; }
  void SetDescription(const std::string& value) { this->description_ = value; }

  const Coverage& GetCoverage() const { return coverage_; }
  Coverage& GetMutableCoverage() { return coverage_; }
  void SetCoverage(const Coverage& value) { this->coverage_ = value; }

  const Owner& GetOwner() const { return owner_; }
  Owner& GetMutableOwner() { return owner_; }
  void SetOwner(const Owner& value) { this->owner_ = value; }

  const std::vector<std::string>& GetTags() const { return tags_; }
  std::vector<std::string>& GetMutableTags() { return tags_; }
  void SetTags(const std::vector<std::string>& value) { this->tags_ = value; }

  const std::vector<std::string>& GetBillingTags() const {
    return billing_tags_;
  }

  std::vector<std::string>& GetMutableBillingTags() { return billing_tags_; }
  void SetBillingTags(const std::vector<std::string>& value) {
    this->billing_tags_ = value;
  }

  const std::string& GetCreated() const { return created_; }
  std::string& GetMutableCreated() { return created_; }
  void SetCreated(const std::string& value) { this->created_ = value; }

  const std::vector<Layer>& GetLayers() const { return layers_; }
  std::vector<Layer>& GetMutableLayers() { return layers_; }
  void SetLayers(const std::vector<Layer>& value) { this->layers_ = value; }

  const int64_t& GetVersion() const { return version_; }
  int64_t& GetMutableVersion() { return version_; }
  void SetVersion(const int64_t& value) { this->version_ = value; }

  const Notifications& GetNotifications() const { return notifications_; }
  Notifications& GetMutableNotifications() { return notifications_; }
  void SetNotifications(const Notifications& value) {
    this->notifications_ = value;
  }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
