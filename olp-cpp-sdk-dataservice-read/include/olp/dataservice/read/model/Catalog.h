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

#pragma once

#include <string>
#include <vector>

#include <boost/optional.hpp>

#include <olp/dataservice/read/DataServiceReadApi.h>

namespace olp {
namespace dataservice {
namespace read {
namespace model {
/**
 * @brief Geographic areas that the catalog covers.
 */
class DATASERVICE_READ_API Coverage {
 public:
  Coverage() = default;
  virtual ~Coverage() = default;

 private:
  std::vector<std::string> admin_areas_;

 public:
  /**
   * @brief Gets the string of catalog administrative areas.
   *
   * The string contains a list of ISO 3166 two-letter codes for countries and
   * regions optionally followed by codes for subdivisions (1–3 characters),
   * such as DE, PL, or CN-HK.
   *
   * @return The string of catalog administrative areas.
   */
  const std::vector<std::string>& GetAdminAreas() const { return admin_areas_; }
  /**
   * @brief Gets a mutable reference to the catalog administrative areas.
   *
   * @see `GetAdminAreas` for information on the catalog administrative areas.
   *
   * @return The mutable reference to the catalog administrative areas.
   */
  std::vector<std::string>& GetMutableAdminAreas() { return admin_areas_; }
  /**
   * @brief Sets the catalog administrative areas.
   *
   * @see `GetAdminAreas` for information on the catalog administrative areas.
   *
   * @param value The catalog administrative areas.
   */
  void SetAdminAreas(const std::vector<std::string>& value) {
    this->admin_areas_ = value;
  }
};

/**
 * @brief A model that represents a definition of an index field.
 */
class DATASERVICE_READ_API IndexDefinition {
 public:
  IndexDefinition() = default;
  virtual ~IndexDefinition() = default;

 private:
  std::string name_;
  std::string type_;
  int64_t duration_{0};
  int64_t zoomLevel_{0};

 public:
  /**
   * @brief Gets the short name of the index field.
   *
   * @return The short name of the index field.
   */
  const std::string& GetName() const { return name_; }
  /**
   * @brief Gets a mutable reference to the short name of the index field.
   *
   * @return The mutable reference to the short name of the index field.
   */
  std::string& GetMutableName() { return name_; }
  /**
   * @brief Sets the short name of the index field.
   *
   * @param value The short name of the index field.
   */
  void SetName(const std::string& value) { this->name_ = value; }

  /**
   * @brief Gets the type of data availability that this layer provides.
   *
   * @return The data availability type.
   */
  const std::string& GetType() const { return type_; }
  /**
   * @brief Gets a mutable reference to the type of data availability that
   * this layer provides.
   *
   * @return The mutable reference to the data availability type.
   */
  std::string& GetMutableType() { return type_; }
  /**
   * @brief Sets the data availability type.
   *
   * @param value The data availability type.
   */
  void SetType(const std::string& value) { this->type_ = value; }

  /**
   * @brief Gets the duration of the time window in milliseconds.
   *
   * The time window represents the time slice and denotes the finest time
   * granularity at which the data will be indexed and later queried.
   * The duration is between 10 minutes and 1 day in milliseconds.
   *
   * @return The duration of the time window in milliseconds.
   */
  const int64_t& GetDuration() const { return duration_; }
  /**
   * @brief Gets a mutable reference to the duration of the time window in
   * milliseconds.
   *
   * @see `GetDuration` for information on the duration of the time
   * window.
   *
   * @return The mutable reference to the duration of the time window in
   * milliseconds.
   */
  int64_t& GetMutableDuration() { return duration_; }
  /**
   * @brief Sets the duration of the time window.
   *
   * @see `GetDuration` for information on the duration of the time
   * window.
   *
   * @param value The duration of the time window in milliseconds.
   */
  void SetDuration(const int64_t& value) { this->duration_ = value; }

  /**
   * @brief Gets the tile size.
   *
   * @return The tile size.
   */
  const int64_t& GetZoomLevel() const { return zoomLevel_; }
  /**
   * @brief Gets a mutable reference to the tile size.
   *
   * @return The mutable reference to the tile size.
   */
  int64_t& GetMutableZoomLevel() { return zoomLevel_; }
  /**
   * @brief Sets the tile size.
   *
   * @param value The tile size.
   */
  void SetZoomLevel(const int64_t& value) { this->zoomLevel_ = value; }
};

/**
 * @brief A model that represents index properties.
 */
class DATASERVICE_READ_API IndexProperties {
 public:
  IndexProperties() = default;
  virtual ~IndexProperties() = default;

 private:
  std::string ttl_;
  std::vector<IndexDefinition> index_definitions_;

 public:
  /**
   * @brief Gets the expiry time (in milliseconds) for data in the index layer.
   *
   * The default value is 604800000 (7 days).
   *
   * @return The expiry time (in milliseconds) for the data in the index layer.
   */
  const std::string& GetTtl() const { return ttl_; }
  /**
   * @brief Gets a mutable reference to the expiry time for data in the index
   * layer.
   *
   * @see `GetTtl` for information of the expiry time.
   *
   * @return The mutable reference to the expiry time for the data in the index
   * layer.
   */
  std::string& GetMutableTtl() { return ttl_; }
  /**
   * @brief Sets the expiry time for the data in the index layer.
   *
   * @see `GetTtl` for information of the expiry time.
   *
   * @param value The expiry time for the data in the index layer.
   */
  void SetTtl(const std::string& value) { this->ttl_ = value; }

  /**
   * @brief Gets the `IndexDefinition` instance.
   *
   * @return The `IndexDefinition` instance.
   */
  const std::vector<IndexDefinition>& GetIndexDefinitions() const {
    return index_definitions_;
  }
  /**
   * @brief Gets a mutable reference to the `IndexDefinition` instance.
   *
   * @return The mutable reference to the `IndexDefinition` instance.
   */
  std::vector<IndexDefinition>& GetMutableIndexDefinitions() {
    return index_definitions_;
  }
  /**
   * @brief Sets the `IndexDefinition` instance.
   *
   * @param value The `IndexDefinition` instance.
   */
  void SetIndexDefinitions(const std::vector<IndexDefinition>& value) {
    this->index_definitions_ = value;
  }
};

/**
 * @brief Contains information on a user or application that initially created
 * the catalog.
 */
class DATASERVICE_READ_API Creator {
 public:
  Creator() = default;
  virtual ~Creator() = default;

 private:
  std::string id_;

 public:
  /**
   * @brief Gets the unique ID of the user or application that initially created
   * the catalog.
   *
   * @return The ID of the user or application that initially created
   * the catalog.
   */
  const std::string& GetId() const { return id_; }
  /**
   * @brief Gets a mutable reference to the ID of the user or application that
   * initially created the catalog.
   *
   * @return The mutable reference to the ID of the user or application that
   * initially created the catalog.
   */
  std::string& GetMutableId() { return id_; }
  /**
   * @brief Sets the ID of the user or application that initially created
   * the catalog.
   *
   * @param value The ID of the user or application that initially created
   * the catalog.
   */
  void SetId(const std::string& value) { this->id_ = value; }
};

/**
 * @brief Contains information on the catalog creator and customer organisation
 * that is related to the catalog.
 */
class DATASERVICE_READ_API Owner {
 public:
  Owner() = default;
  virtual ~Owner() = default;

 private:
  Creator creator_;
  Creator organisation_;

 public:
  /**
   * @brief Gets the `Creator` instance.
   *
   * @return The `Creator` instance.
   */
  const Creator& GetCreator() const { return creator_; }
  /**
   * @brief Gets a mutable reference to the `Creator` instance.
   *
   * @return The mutable reference to the `Creator` instance.
   */
  Creator& GetMutableCreator() { return creator_; }
  /**
   * @brief Sets `Creator` instance.
   *
   * @param value The `Creator` instance.
   */
  void SetCreator(const Creator& value) { this->creator_ = value; }

  /**
   * @brief Gets the ID of the customer organisation that is related to
   * this catalog.
   *
   * @return The ID of the customer organisation that is related to
   * this catalog.
   */
  const Creator& GetOrganisation() const { return organisation_; }
  /**
   * @brief Gets a mutable reference to the ID of the customer organisation that
   * is related to this catalog.
   *
   * @return The mutable reference to the ID of the customer organisation that
   * is related to this catalog.
   */
  Creator& GetMutableOrganisation() { return organisation_; }
  /**
   * @brief Sets ID of the customer organisation.
   *
   * @param value The ID of the customer organisation that is related to
   * this catalog.
   */
  void SetOrganisation(const Creator& value) { this->organisation_ = value; }
};

/**
 * @brief A paritioning scheme of the catalog.
 */
class DATASERVICE_READ_API Partitioning {
 public:
  Partitioning() = default;
  virtual ~Partitioning() = default;

 private:
  std::string scheme_;
  std::vector<int64_t> tile_levels_;

 public:
  /**
   * @brief Gets the name of the catalog partitioning scheme.
   *
   * The partitioning scheme can be generic or HERE tile.
   *
   * @see [Generic
   * Partitioninc](https://developer.here.com/olp/documentation/get-started/dev_guide/shared_content/topics/olp/concepts/partitions.html#generic-partitioning)
   * and [HERE Tile
   * Partitioning](https://developer.here.com/olp/documentation/get-started/dev_guide/shared_content/topics/olp/concepts/partitions.html#here-tile-partitioning)
   * sections in the Get Started guide.
   *
   * @return The name of the catalog partitioning scheme.
   */
  const std::string& GetScheme() const { return scheme_; }
  /**
   * @brief Gets a mutable reference to the name of the catalog partitioning
   * scheme.
   *
   * @see `GetScheme` for information on the catalog partitioning
   * scheme.
   *
   * @return The mutable reference to the name of the catalog partitioning
   * scheme.
   */
  std::string& GetMutableScheme() { return scheme_; }
  /**
   * @brief Sets the name of the catalog partitioning scheme.
   *
   * @see `GetScheme` for information on the catalog partitioning
   * scheme.
   *
   * @param value The name of the catalog partitioning scheme.
   */
  void SetScheme(const std::string& value) { this->scheme_ = value; }

  /**
   * @brief Gets the list of the quad tree tile levels that contain data
   * partitions.
   *
   * Only used if the partitioning scheme is HERE Tile.
   *
   * @return The list of the quad tree tile levels that contain data partitions.
   */
  const std::vector<int64_t>& GetTileLevels() const { return tile_levels_; }
  /**
   * @brief Gets a mutable reference to the list of the quad tree tile levels
   * that contain data partitions.
   *
   * Only used if the partitioning scheme is HERE Tile.
   *
   * @return The mutable reference to the list of the quad tree tile levels that
   * contain data partitions.
   */
  std::vector<int64_t>& GetMutableTileLevels() { return tile_levels_; }
  /**
   * @brief Sets the list of the quadtree tile levels that contain data
   * partitions.
   *
   * @param value The list of the quadtree tile levels that contain data
   * partitions.
   */
  void SetTileLevels(const std::vector<int64_t>& value) {
    this->tile_levels_ = value;
  }
};

/**
 * @brief Describes a HERE Resource Name (HRN) of a layer schema.
 */
class DATASERVICE_READ_API Schema {
 public:
  Schema() = default;
  virtual ~Schema() = default;

 private:
  std::string hrn_;

 public:
  /**
   * @brief Gets the HRN of the layer schema.
   *
   * @return The HRN of the layer schema.
   */
  const std::string& GetHrn() const { return hrn_; }
  /**
   * @brief Gets a mutable reference to the HRN of the layer schema.
   *
   * @return The mutable reference to the HRN of the layer schema.
   */
  std::string& GetMutableHrn() { return hrn_; }
  /**
   * @brief Sets the HRN of the layer schema.
   *
   * @param value The HRN of the layer schema.
   */
  void SetHrn(const std::string& value) { this->hrn_ = value; }
};

/**
 * @brief Properties that define the scale of the required streaming service.
 */
class DATASERVICE_READ_API StreamProperties {
 public:
  StreamProperties() = default;
  virtual ~StreamProperties() = default;

 private:
  int64_t data_in_throughput_mbps_{0};
  int64_t data_out_throughput_mbps_{0};

 public:
  /**
   * @brief Gets the maximum throughput for the incoming data expressed in
   * megabytes per second.
   *
   * Throttling occurs when the inbound rate exceeds the maximum inbound
   * throughput. The default value is 4 MBps. The maximum value is 32 MBps.
   *
   * @return The maximum throughput for the incoming data.
   */
  const int64_t& GetDataInThroughputMbps() const {
    return data_in_throughput_mbps_;
  }
  /**
   * @brief Gets a mutable reference to the maximum throughput for the incoming
   * data.
   *
   * @see `GetDataInThroughputMbps` for information on the maximum throughput
   * for the incoming data.
   *
   * @return The mutable reference to the maximum throughput for the incoming
   * data.
   */
  int64_t& GetMutableDataInThroughputMbps() { return data_in_throughput_mbps_; }
  /**
   * @brief Sets the maximum throughput for the incoming data.
   *
   * @see `GetDataInThroughputMbps` for information on the maximum
   * throughput for the incoming data.
   *
   * @param value The maximum throughput for the incoming data.
   */
  void SetDataInThroughputMbps(const int64_t& value) {
    this->data_in_throughput_mbps_ = value;
  }

  /**
   * @brief Gets the maximum throughput for the outgoing data expressed in
   * megabytes per second.
   *
   * Throttling occurs when the total outbound rate to all consumers
   * exceeds the maximum outbound throughput. The default value is 8 MBps.
   * The maximum value is 64 MBps.
   *
   * @return The maximum throughput for the outgoing data.
   */
  const int64_t& GetDataOutThroughputMbps() const {
    return data_out_throughput_mbps_;
  }
  /**
   * @brief Gets a mutable reference to the maximum throughput for the outgoing
   * data.
   *
   * @see `GetDataOutThroughputMbps` for information on the maximum throughput
   * for the outgoing data.
   *
   * @return The mutable reference to the maximum throughput for the outgoing
   * data.
   */
  int64_t& GetMutableDataOutThroughputMbps() {
    return data_out_throughput_mbps_;
  }
  /**
   * @brief Sets the maximum throughput for the outgoing data.
   *
   * @see `GetDataOutThroughputMbps` for information on the maximum throughput
   * for the outgoing data.
   *
   * @param value The maximum throughput for outgoing data
   */
  void SetDataOutThroughputMbps(const int64_t& value) {
    this->data_out_throughput_mbps_ = value;
  }
};

/**
 * @brief An encryption scheme of the catalog.
 */
class DATASERVICE_READ_API Encryption {
 public:
  Encryption() = default;
  virtual ~Encryption() = default;

 private:
  std::string algorithm_;

 public:
  /**
   * @brief Gets the encryption algorithm.
   *
   * @return The encryption algorithm.
   */
  const std::string& GetAlgorithm() const { return algorithm_; }
  /**
   * @brief Gets a mutable reference to the encryption algorithm.
   *
   * @return The mutable reference to the encryption algorithm.
   */
  std::string& GetMutableAlgorithm() { return algorithm_; }
  /**
   * @brief Sets the encryption algorithm.
   *
   * @param value The encryption algorithm.
   */
  void SetAlgorithm(const std::string& value) { this->algorithm_ = value; }
};

/**
 * @brief Describes storage details of a catalog layer.
 */
class DATASERVICE_READ_API Volume {
 public:
  Volume() = default;
  virtual ~Volume() = default;

 private:
  std::string volume_type_;
  std::string max_memory_policy_;
  std::string package_type_;
  Encryption encryption_;

 public:
  /**
   * @brief Gets the volume type that is used to store the layer data content.
   *
   * @return The volume type that is used to store the layer data content.
   */
  const std::string& GetVolumeType() const { return volume_type_; }
  /**
   * @brief Gets a mutable reference to the volume type that is used to store
   * the layer data content.
   *
   * @return The mutable reference to the volume type that is used to store
   * the layer data content.
   */
  std::string& GetMutableVolumeType() { return volume_type_; }
  /**
   * @brief Sets the volume type.
   *
   * @param value The volume type that is used to store the layer data content.
   */
  void SetVolumeType(const std::string& value) { this->volume_type_ = value; }

  /**
   * @brief Gets the keys eviction policy when the memory limit for the layer is
   * reached.
   *
   * @return The keys eviction policy.
   */
  const std::string& GetMaxMemoryPolicy() const { return max_memory_policy_; }
  /**
   * @brief Gets a mutable reference to the keys eviction policy when the memory
   * limit for the layer is reached.
   *
   * @return The mutable reference to the keys eviction policy.
   */
  std::string& GetMutableMaxMemoryPolicy() { return max_memory_policy_; }
  /**
   * @brief Sets the keys eviction policy.
   *
   * @param value The keys eviction policy.
   */
  void SetMaxMemoryPolicy(const std::string& value) {
    this->max_memory_policy_ = value;
  }

  /**
   * @brief Gets the initial package type (capacity) of the layer.
   *
   * @return The package type (capacity) of the layer.
   */
  const std::string& GetPackageType() const { return package_type_; }
  /**
   * @brief Gets a mutable reference to the package type (capacity) of
   * the layer.
   *
   * @return The mutable reference to the package type (capacity) of the layer.
   */
  std::string& GetMutablePackageType() { return package_type_; }
  /**
   * @brief Sets the package type (capacity) of the layer.
   *
   * @param value The package type (capacity) of the layer.
   */
  void SetPackageType(const std::string& value) { this->package_type_ = value; }

  /**
   * @brief Gets the `Encryption` instance.
   *
   * @return The `Encryption` instance.
   */
  const Encryption& GetEncryption() const { return encryption_; }
  /**
   * @brief Gets a mutable reference to the `Encryption` instance.
   *
   * @return The mutable reference to the `Encryption` instance.
   */
  Encryption& GetMutableEncryption() { return encryption_; }
  /**
   * @brief Sets the `Encryption` instance.
   *
   * @param value The `Encryption` instance.
   */
  void SetEncryption(const Encryption& value) { this->encryption_ = value; }
};

/**
 * @brief A layer of a catalog.
 */
class DATASERVICE_READ_API Layer {
 public:
  Layer() = default;
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
  boost::optional<int64_t> ttl_;
  IndexProperties index_properties_;
  StreamProperties stream_properties_;
  Volume volume_;

 public:
  /**
   * @brief Gets the ID that is used to refer to this layer programmatically.
   *
   * @return The layer ID.
   */
  const std::string& GetId() const { return id_; }
  /**
   * @brief Gets a mutable reference to the ID that is used to refer to
   * this layer programmatically.
   *
   * @return The mutable reference to the layer ID.
   */
  std::string& GetMutableId() { return id_; }
  /**
   * @brief Sets the layer ID.
   *
   * @param value The layer ID.
   */
  void SetId(const std::string& value) { this->id_ = value; }

  /**
   * @brief Gets the layer display name.
   *
   * @return The layer display name.
   */
  const std::string& GetName() const { return name_; }
  /**
   * @brief Gets a mutable refrence to the layer display name.
   *
   * @return The mutable reference to the layer display name.
   */
  std::string& GetMutableName() { return name_; }
  /**
   * @brief Sets the layer display name.
   *
   * @param value The layer display name.
   */
  void SetName(const std::string& value) { this->name_ = value; }

  /**
   * @brief Gets the one-sentence summary of the layer.
   *
   * You can see this summary when you browse a list of layers on the platform
   * portal.
   *
   * @return The one-sentence summary of the layer.
   */
  const std::string& GetSummary() const { return summary_; }
  /**
   * @brief Gets a mutable reference to the one-sentence summary of the layer.
   *
   * @see `GetSummary` for information on the layer summary.
   *
   * @return The mutable reference to the one-sentence summary of the layer.
   */
  std::string& GetMutableSummary() { return summary_; }
  /**
   * @brief Sets the layer summary.
   *
   * @see `GetSummary` for information on the layer summary.
   *
   * @param value The one-sentence summary of the layer.
   */
  void SetSummary(const std::string& value) { this->summary_ = value; }

  /**
   * @brief Gets the detailed description of the layer.
   *
   * This information appears on the **Overview** tab when you open a layer in
   * the platform portal.
   *
   * @return The detailed description of the layer.
   */
  const std::string& GetDescription() const { return description_; }
  /**
   * @brief Gets a mutable reference to the detailed description of the layer.
   *
   * @return The mutable reference to the detailed description of the layer.
   */
  std::string& GetMutableDescription() { return description_; }
  /**
   * @brief Sets the detailed description of the layer.
   *
   * @param value The detailed description of the layer.
   */
  void SetDescription(const std::string& value) { this->description_ = value; }

  /**
   * @brief Gets the `Owner` instance.
   *
   * @return The `Owner` instance.
   */
  const Owner& GetOwner() const { return owner_; }
  /**
   * @brief Gets a mutable reference to the `Owner` instance.
   *
   * @return The mutable reference to the `Owner` instance.
   */
  Owner& GetMutableOwner() { return owner_; }
  /**
   * @brief Sets the `Owner` instance.
   *
   * @param value The `Owner` instance.
   */
  void SetOwner(const Owner& value) { this->owner_ = value; }

  /**
   * @brief Gets the `Coverage` instance.
   *
   * @return The `Coverage` instance.
   */
  const Coverage& GetCoverage() const { return coverage_; }
  /**
   * @brief Gets a mutable reference to the `Coverage` instance.
   *
   * @return The mutable reference to the `Coverage` instance.
   */
  Coverage& GetMutableCoverage() { return coverage_; }
  /**
   * @brief Sets the `Coverage` instance.
   *
   * @param value The `Coverage` instance.
   */
  void SetCoverage(const Coverage& value) { this->coverage_ = value; }

  /**
   * @brief Gets the `Schema` instance.
   *
   * @return The `Schema` instance.
   */
  const Schema& GetSchema() const { return schema_; }
  /**
   * @brief Gets a mutable reference to the `Schema` instance.
   *
   * @return The mutable reference to the `Schema` instance.
   */
  Schema& GetMutableSchema() { return schema_; }
  /**
   * @brief Sets the `Schema` instance.
   *
   * @param value The `Schema` instance.
   */
  void SetSchema(const Schema& value) { this->schema_ = value; }

  /**
   * @brief Gets the data of the MIME type that is stored in the layer.
   *
   * @return The data of the MIME type.
   */
  const std::string& GetContentType() const { return content_type_; }
  /**
   * @brief Gets a mutable reference to the data of the MIME type that is stored
   * in the layer.
   *
   * @return The mutable reference to the data of the MIME type.
   */
  std::string& GetMutableContentType() { return content_type_; }
  /**
   * @brief Sets the data of the MIME type.
   *
   * @param value The data of the MIME type that is stored in the layer.
   */
  void SetContentType(const std::string& value) { this->content_type_ = value; }

  /**
   * @brief Gets the compressed data from the layer.
   *
   * @return The compressed data.
   */
  const std::string& GetContentEncoding() const { return content_encoding_; }
  /**
   * @brief Gets a mutable reference to the compressed data from the layer.
   *
   * @return The mutable reference to the compressed data.
   */
  std::string& GetMutableContentEncoding() { return content_encoding_; }
  /**
   * @brief Sets the compressed data.
   *
   * @param value The compressed data.
   */
  void SetContentEncoding(const std::string& value) {
    this->content_encoding_ = value;
  }

  /**
   * @brief Gets the `Partitioning` instance.
   *
   * @return The `Partitioning` instance.
   */
  const Partitioning& GetPartitioning() const { return partitioning_; }
  /**
   * @brief Gets a mutable reference to the `Partitioning` instance.
   *
   * @return The mutable reference to the `Partitioning` instance.
   */
  Partitioning& GetMutablePartitioning() { return partitioning_; }
  /**
   * @brief Sets the `Partitioning` instance.
   *
   * @param value The `Partitioning` instance.
   */
  void SetPartitioning(const Partitioning& value) {
    this->partitioning_ = value;
  }

  /**
   * @brief Gets the type of data availability that this layer provides.
   *
   * @return The data availability type.
   */
  const std::string& GetLayerType() const { return layer_type_; }
  /**
   * @brief Gets a mutable reference to the type of data availability that
   * this layer provides.
   *
   * @return The mutable reference to the data availability type.
   */
  std::string& GetMutableLayerType() { return layer_type_; }
  /**
   * @brief Sets the data availability type.
   *
   * @param value The data availability type.
   */
  void SetLayerType(const std::string& value) { this->layer_type_ = value; }

  /**
   * @brief Gets the digest algorithm used to calculate the checksum for
   * the partitions in this layer.
   *
   * If specified, you can assume that all partitions in the layer, at every
   * version, were calculated using this algorithm.
   *
   * @note It is the responsibility of the data publisher to use this algorithm
   * to calculate partition checksums. The Open Location Platform (OLP) does not
   * verify that the specified algorithm was actually used.
   *
   * @return The digest algorithm used to calculate the checksum for the
   * partitions in this layer.
   */
  const std::string& GetDigest() const { return digest_; }
  /**
   * @brief Gets a mutable reference to the digest algorithm used to calculate
   * the checksum for the partitions in this layer.
   *
   * @see `GetDigest` for information on the digest algorithm.
   *
   * @return The mutable reference to the digest algorithm used to calculate
   * the checksum for the partitions in this layer.
   */
  std::string& GetMutableDigest() { return digest_; }
  /**
   * @brief Sets the digest algorithm used to calculate the checksum for
   * the partitions in this layer.
   *
   * @see `GetDigest` for information on the digest algorithm.
   *
   * @param value The digest algorithm used to calculate the checksum for
   * the partitions in this layer.
   */
  void SetDigest(const std::string& value) { this->digest_ = value; }

  /**
   * @brief Gets the keywords that help to find the layer on the platform
   * portal.
   *
   * @return The keywords that help to find the layer.
   */
  const std::vector<std::string>& GetTags() const { return tags_; }
  /**
   * @brief Gets a mutable reference to the keywords that help to find the layer
   * on the platform portal.
   *
   * @return The mutable reference to the keywords that help to find the layer.
   */
  std::vector<std::string>& GetMutableTags() { return tags_; }
  /**
   * @brief Sets the keywords that help to find the layer.
   *
   * @param value The keywords that help to find the layer on the platform
   * portal.
   */
  void SetTags(const std::vector<std::string>& value) { this->tags_ = value; }

  /**
   * @brief Gets the list of billing tags that are used to group billing
   * records.
   *
   * The billing tag is an optional free-form tag that is used for grouping
   * billing records. If supplied, it must be 4–16 characters
   * long and contain only alphanumeric ASCII characters [A-Za-z0-9].
   *
   * @return The list of billing tags.
   */
  const std::vector<std::string>& GetBillingTags() const {
    return billing_tags_;
  }
  /**
   * @brief Gets a mutable reference to the list of billing tags that are used
   * to group billing records.
   *
   * @see `GetBillingTags` for information on the billing tags.
   *
   * @return The mutable reference to the list of billing tags.
   */
  std::vector<std::string>& GetMutableBillingTags() { return billing_tags_; }
  /**
   * @brief Sets the list of billing tags.
   *
   * @see `GetBillingTags` for information on the billing tags.
   *
   * @param value The list of billing tags that are used to group billing
   * records.
   */
  void SetBillingTags(const std::vector<std::string>& value) {
    this->billing_tags_ = value;
  }

  /**
   * @brief The expiry time (in milliseconds) for data in this layer.
   *
   * Data is automatically removed when the specified time limit elapses.
   * For volatile layers, the TTL value must be between 60000 (1 minute) and
   * 604800000 (7 days). For stream layers, the TTL value must be between
   * 600000 (10 minutes) and 259200000 (72 hours).
   *
   * @return The expiry time (in milliseconds) for data in this layer.
   */
  const boost::optional<int64_t>& GetTtl() const { return ttl_; }
  /**
   * @brief Gets a mutable reference to the expiry time for data in this layer.
   *
   * @see `GetTtl` for information on the expiry time.
   *
   * @return The mutable reference to the expiry time for data in
   * this layer.
   */
  boost::optional<int64_t>& GetMutableTtl() { return ttl_; }
  /**
   * @brief Sets the expiry time for data in this layer.
   *
   * @see `GetTtl` for information on the expiry time.
   *
   * @param value The expiry time for data in this layer.
   */
  void SetTtl(const boost::optional<int64_t>& value) { this->ttl_ = value; }

  /**
   * @brief Gets the `IndexProperties` instance.
   *
   * @return The `IndexProperties` instance.
   */
  const IndexProperties& GetIndexProperties() const {
    return index_properties_;
  }
  /**
   * @brief Gets a mutable reference to the `IndexProperties` instance.
   *
   * @return The mutable reference to the `IndexProperties` instance.
   */
  IndexProperties& GetMutableIndexProperties() { return index_properties_; }
  /**
   * @brief Sets the `IndexProperties` instance.
   *
   * @param value The `IndexProperties` instance.
   */
  void SetIndexProperties(const IndexProperties& value) {
    this->index_properties_ = value;
  }

  /**
   * @brief Gets the `StreamProperties` instance.
   *
   * @return The `StreamProperties` instance.
   */
  const StreamProperties& GetStreamProperties() const {
    return stream_properties_;
  }
  /**
   * @brief Gets a mutable reference to the `StreamProperties` instance.
   *
   * @return The mutable reference to the `StreamProperties` instance.
   */
  StreamProperties& GetMutableStreamProperties() { return stream_properties_; }
  /**
   * @brief Sets the `StreamProperties` instance
   *
   * @param value The `StreamProperties` instance
   */
  void SetStreamProperties(const StreamProperties& value) {
    this->stream_properties_ = value;
  }

  /**
   * @brief Gets the `Volume` instance.
   *
   * @return The `Volume` instance.
   */
  const Volume& GetVolume() const { return volume_; }
  /**
   * @brief Gets a mutable reference to the `Volume` instance.
   *
   * @return The mutable reference to the `Volume` instance.
   */
  Volume& GetMutableVolume() { return volume_; }
  /**
   * @brief Sets the `Volume` instance.
   *
   * @param value The `Volume` instance.
   */
  void SetVolume(const Volume& value) { this->volume_ = value; }
};

/**
 * @brief Catalog notifications.
 */
class DATASERVICE_READ_API Notifications {
 public:
  Notifications() = default;
  virtual ~Notifications() = default;

 private:
  bool enabled_{false};

 public:
  /**
   * @brief Checks whether the notifications are written to the notification
   * stream each time the catalog version changes.
   *
   * @return If set to true, notifications are written to the notification
   * stream each time the catalog version changes.
   */
  const bool& GetEnabled() const { return enabled_; }
  /**
   * @brief Checks whether the notifications are written to the notification
   * stream each time the catalog version changes and returns a mutable
   * reference.
   *
   * @return If set to true, notifications are written to the notification
   * stream each time the catalog version changes.
   */
  bool& GetMutableEnabled() { return enabled_; }
  /**
   * @brief Sets the notifications for the catalog.
   *
   * @param value If set to true, notifications are written to the notification
   * stream each time the catalog version changes.
   */
  void SetEnabled(const bool& value) { this->enabled_ = value; }
};

/**
 * @brief A model that represents a catalog.
 */
class DATASERVICE_READ_API Catalog {
 public:
  Catalog() = default;
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
  /**
   * @brief Gets the ID that is used to refer to this catalog
   * programmatically.
   *
   * All catalog IDs must be unique across all catalogs in the Open Location
   * Platform(OLP). Do not put private information in the catalog ID.
   * The catalog ID forms a portion of the catalog HERE Resource Name (HRN), and
   * HRNs are visible to other users.
   *
   * @return The catalog ID.
   */
  const std::string& GetId() const { return id_; }
  /**
   * @brief Gets a mutable reference to the ID that is used to refer to
   * this catalog programmatically.
   *
   * @see `GetId` for information on the catalog ID.
   *
   * @return The mutable reference to the catalog ID.
   */
  std::string& GetMutableId() { return id_; }
  /**
   * @brief Sets the catalog ID.
   *
   * @see `GetId` for information on the catalog ID.
   *
   * @param value The catalog ID.
   */
  void SetId(const std::string& value) { this->id_ = value; }

  /**
   * @brief Gets the HERE Resource Name (HRN) of the catalog.
   *
   * @return The catalog HRN.
   */
  const std::string& GetHrn() const { return hrn_; }
  /**
   * @brief Gets a mutable reference to the HRN of the catalog.
   *
   * @return The mutable reference to the catalog HRN.
   */
  std::string& GetMutableHrn() { return hrn_; }
  /**
   * @brief Sets the catalog HRN.
   *
   * @param value The catalog HRN.
   */
  void SetHrn(const std::string& value) { this->hrn_ = value; }

  /**
   * @brief Gets the short name of the catalog.
   *
   * @return The catalog short name.
   */
  const std::string& GetName() const { return name_; }
  /**
   * @brief Gets a mutable reference to the catalog short name.
   *
   * @return The mutable reference to the catalog short name.
   */
  std::string& GetMutableName() { return name_; }
  /**
   * @brief Sets the catalog short name.
   *
   * @param value The catalog short name.
   */
  void SetName(const std::string& value) { this->name_ = value; }

  /**
   * @brief Gets the one-sentence summary of the catalog.
   *
   * You can see this summary when you browse a list of catalogs on the platform
   * portal.
   *
   * @return The one-sentence summary of the catalog.
   */
  const std::string& GetSummary() const { return summary_; }
  /**
   * @brief Gets a mutable reference to the one-sentence summary of the catalog.
   *
   * @return The mutable reference to the one-sentence summary of the catalog.
   */
  std::string& GetMutableSummary() { return summary_; }
  /**
   * @brief Sets the catalog summary.
   *
   * @param value The one-sentence summary of the catalog.
   */
  void SetSummary(const std::string& value) { this->summary_ = value; }

  /**
   * @brief Gets the detailed description of the catalog.
   *
   * This description appears on the **Overview** tab when you open a catalog in
   * the platform portal.
   *
   * @return The detailed description of the catalog.
   */
  const std::string& GetDescription() const { return description_; }
  /**
   * @brief Gets a mutable reference to the detailed description of the catalog.
   *
   * @see `GetDescription` for more information on the detailed description of
   * the catalog.
   *
   * @return The mutable reference to the detailed description of the catalog.
   */
  std::string& GetMutableDescription() { return description_; }
  /**
   * @brief Sets the detailed description of the catalog.
   *
   * @see `GetDescription` for more information on the detailed description of
   * the catalog.
   *
   * @param value The detailed description of the catalog.
   */
  void SetDescription(const std::string& value) { this->description_ = value; }

  /**
   * @brief Gets the `Coverage` instance.
   *
   * @return The `Coverage` instance.
   */
  const Coverage& GetCoverage() const { return coverage_; }
  /**
   * @brief Gets a mutable reference to the `Coverage` instance.
   *
   * @return The mutable reference to the `Coverage` instance.
   */
  Coverage& GetMutableCoverage() { return coverage_; }
  /**
   * @brief Sets the `Coverage` instance.
   *
   * @param value The `Coverage` instance.
   */
  void SetCoverage(const Coverage& value) { this->coverage_ = value; }

  /**
   * @brief Gets the `Owner` instance.
   *
   * @return The `Owner` instance.
   */
  const Owner& GetOwner() const { return owner_; }
  /**
   * @brief Gets a mutable reference to the `Owner` instance.
   *
   * @return The mutable reference to the `Owner` instance.
   */
  Owner& GetMutableOwner() { return owner_; }
  /**
   * @brief Sets the `Owner` instance.
   *
   * @param value The `Owner` instance.
   */
  void SetOwner(const Owner& value) { this->owner_ = value; }

  /**
   * @brief Gets the keywords that help to find the catalog on the platform
   * portal.
   *
   * @return The keywords that help to find the catalog.
   */
  const std::vector<std::string>& GetTags() const { return tags_; }
  /**
   * @brief Gets a mutable reference to the keywords that help to find
   * the catalog on the platform portal.
   *
   * @return The mutable reference to the keywords that help to find
   * the catalog.
   */
  std::vector<std::string>& GetMutableTags() { return tags_; }
  /**
   * @brief Sets the keywords that help to find the catalog.
   *
   * @param value The keywords that help to find the catalog on the platform
   * portal.
   */
  void SetTags(const std::vector<std::string>& value) { this->tags_ = value; }

  /**
   * @brief Gets the list of billing tags that are used to group billing
   * records.
   *
   * The billing tag is an optional free-form tag that is used for grouping
   * billing records. If supplied, it must be 4–16 characters
   * long and contain only alphanumeric ASCII characters [A-Za-z0-9].
   *
   * @return The list of billing tags.
   */
  const std::vector<std::string>& GetBillingTags() const {
    return billing_tags_;
  }
  /**
   * @brief Gets a mutable reference to the list of billing tags that are used
   * to group billing records.
   *
   * @see `GetBillingTags` for information on the billing tags.
   *
   * @return The mutable reference to the list of billing tags.
   */
  std::vector<std::string>& GetMutableBillingTags() { return billing_tags_; }
  /**
   * @brief Sets the list of billing tags.
   *
   * @see `GetBillingTags` for information on the billing tags.
   *
   * @param value The list of billing tags that are used to group billing
   * records.
   */
  void SetBillingTags(const std::vector<std::string>& value) {
    this->billing_tags_ = value;
  }

  /**
   * @brief Gets the catalog creation date and time.
   *
   * @return The catalog creation date and time.
   */
  const std::string& GetCreated() const { return created_; }
  /**
   * @brief Gets a mutable reference to the catalog creation date and time.
   *
   * @return The mutable reference to the catalog creation date and time.
   */
  std::string& GetMutableCreated() { return created_; }
  /**
   * @brief Sets the catalog creation date and time.
   *
   * @param value The catalog creation date and time.
   */
  void SetCreated(const std::string& value) { this->created_ = value; }

  /**
   * @brief Gets the vector with the `Layer` instance.
   *
   * @return The vector with the `Layer` instance.
   */
  const std::vector<Layer>& GetLayers() const { return layers_; }
  /**
   * @brief Gets a mutable reference to the vector with the `Layer` instance.
   *
   * @return The mutable reference to the vector with the `Layer` instance.
   */
  std::vector<Layer>& GetMutableLayers() { return layers_; }
  /**
   * @brief Sets the vector with the `Layer` instance.
   *
   * @param value The vector with the `Layer` instance.
   */
  void SetLayers(const std::vector<Layer>& value) { this->layers_ = value; }

  /**
   * @brief Gets the version of the catalog configuration.
   *
   * Every change in this number indicates a change in catalog
   * configuration. Examples of changes in catalog configuration include
   * changing catalog parameters and adding layers. Note that the catalog
   * configuration version is not the same as the metadata/data version.
   * Configuration and metadata versions are independent of each other and
   * indicate different kinds of changes.
   *
   * @return The catalog configuration version.
   */
  const int64_t& GetVersion() const { return version_; }
  /**
   * @brief Gets a mutable reference to the version of the catalog
   * configuration.
   *
   * @see `GetVersion` for information on the catalog configuration
   * version.
   *
   * @return The mutable reference to the catalog configuration version.
   */
  int64_t& GetMutableVersion() { return version_; }
  /**
   * @brief Sets the catalog configuration version.
   *
   * @see `GetVersion` for information on the catalog configuration
   * version.
   *
   * @param value The catalog configuration version.
   */
  void SetVersion(const int64_t& value) { this->version_ = value; }

  /**
   * @brief Gets the `Notifications` instance.
   *
   * @return The `Notifications` instance.
   */
  const Notifications& GetNotifications() const { return notifications_; }
  /**
   * @brief Gets a mutable reference to the `Notifications` instance.
   *
   * @return The mutable reference to the `Notifications` instance.
   */
  Notifications& GetMutableNotifications() { return notifications_; }
  /**
   * @brief Sets the `Notifications` instance.
   *
   * @param value The `Notifications` instance.
   */
  void SetNotifications(const Notifications& value) {
    this->notifications_ = value;
  }
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
