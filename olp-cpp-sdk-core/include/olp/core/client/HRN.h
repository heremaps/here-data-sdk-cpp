/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <memory>
#include <string>

#include <olp/core/CoreApi.h>

namespace olp {
namespace client {

/**
 * @brief Allows a Here Resource Name (HRN) to be passed to the operations that
 * require it.
 */
class CORE_API HRN {
 public:
  /**
   * @brief Defines the objects to which the HRN can refer.
   */
  enum ServiceType {
    Unknown,  ///< The service type is unknown.
    Data,     ///< This HRN represents the data catalog.
    Schema,   ///< This HRN represents the schema type.
    Pipeline  ///< This HRN represents the pipeline instance.
  };

  /**
   * @brief Creates the `HRN` instance from a string.
   *
   * The passed string must start with `hrn:`.
   *
   * @param input The `HRN` string.
   *
   * @return The `HRN` instance created from the string.
   */
  static HRN FromString(const std::string& input);

  /**
   * @brief Creates the unique `HRN` instance from a string.
   *
   * The passed string must start with `hrn:`.
   *
   * @param input The `HRN` string.
   *
   * @return The unique `HRN` instance created from the string.
   */
  static std::unique_ptr<HRN> UniqueFromString(const std::string& input);

  /**
   * @brief Default constructor which creates an empty invalid HRN.
   */
  HRN() = default;

  /**
   * @brief Creates the `HRN` instance from a string.
   *
   * The passed string must start with `hrn:`.
   *
   * @param input The `HRN` string.
   */
  explicit HRN(const std::string& input);

  /**
   * @brief Checks whether the values of the `HRN` parameter are
   * the same as the values of the `rhs` parameter.
   *
   * @param rhs The `HRN` instance.
   *
   * @return True if the values of the `HRN` and `rhs` parameters are
   * equal; false otherwise.
   */
  bool operator==(const HRN& rhs) const;

  /**
   * @brief Checks whether the values of the `HRN` parameter are not
   * the same as the values of the `rhs` parameter.
   *
   * @param rhs The `HRN` instance.
   *
   * @return True if the values of the `HRN` and `rhs` parameters are
   * not equal; false otherwise.
   */
  bool operator!=(const HRN& rhs) const;

  /**
   * @brief Checks whether the service type fields of the `HRN` instance are not
   * empty.
   *
   * @return True if the service type fields of the `HRN` instance are not
   * empty; false otherwise.
   */
  explicit operator bool() const;

  /**
   * @brief Checks whether any of the service type fields of the `HRN` instance
   * are empty.
   *
   * @return True if at least one of the service type fields of the `HRN`
   * instance is empty; false otherwise.
   */
  bool IsNull() const;

  /**
   * @brief Converts this HRN to a string.
   *
   * Example: `hrn:partition:service:region:account:resource`
   *
   * @return The `HRN` string that has the `hrn:` prefix.
   */
  std::string ToString() const;

  /**
   * @brief Converts this HRN to a string catalog ID.
   *
   * @note Only relevant if the HRN has `ServiceType == Data`.
   *
   * @return The catalog ID that has the `hrn:` prefix.
   */
  std::string ToCatalogHRNString() const;

  /**
   * @brief Returns the partitions of this HRN.
   *
   * @note Must be valid when `ServiceType == Data` or when `ServiceType ==
   * Pipeline`.
   *
   * @return The partition of this HRN.
   */
  const std::string& GetPartition() const { return partition_; }

  /**
   * @brief Returns the service type of this HRN.
   *
   * @return The service type enum.
   */
  ServiceType GetService() const { return service_; }

  /**
   * @brief Returns the region of this HRN.
   *
   * @return The region of this HRN.
   */
  const std::string& GetRegion() const { return region_; }

  /**
   * @brief Returns the account of this HRN.
   *
   * @return The account associated with this HRN.
   */
  const std::string& GetAccount() const { return account_; }

  /**
   * @brief Returns the catalog ID.
   *
   * @note Must be valid in case `ServiceType == Data`.
   *
   * @return The catalog ID.
   */
  const std::string& GetCatalogId() const { return catalog_id_; }

  /**
   * @brief Returns the layer ID.
   *
   * @note This parameter is optional and not always used.
   *
   * @return The layer ID or a empty string if not set.
   */
  const std::string& GetLayerId() const { return layer_id_; }

  /**
   * @brief Returns the group ID.
   *
   * @return The group ID or a empty string in case `ServiceType != Schema`.
   */
  const std::string& GetGroupId() const { return group_id_; }

  /**
   * @brief Retruns the schema name.
   *
   * @return The schema name or a empty string in case `ServiceType != Schema`.
   */
  const std::string& GetSchemaName() const { return schema_name_; }

  /**
   * @brief Returns the catalog version.
   *
   * @return The catalog version or a empty string in case `ServiceType !=
   * Schema`.
   */
  const std::string& GetVersion() const { return version_; }

  /**
   * @brief Returns the pipeline ID.
   *
   * @return The pipeline ID or and empty string in case `HRNServiceType !=
   * Pipeline` and `ServiceType != Schema`.
   */
  const std::string& GetPipelineId() const { return pipeline_id_; }

 private:
  /// The partition of the HRN. Must be valid when `ServiceType == Data` or when
  /// `ServiceType == Pipeline`.
  std::string partition_;

  /// The service type of the HRN.
  ServiceType service_{ServiceType::Unknown};

  /// The region of the HRN.
  std::string region_;

  /// The account of the HRN.
  std::string account_;

  /// The catalog ID. Must be valid when `ServiceType == Data`.
  std::string catalog_id_;

  /// (Optional) The layer ID.
  std::string layer_id_;

  /// The group ID. Must be valid if `ServiceType == Schema`.
  std::string group_id_;

  /// The schema name. Must be valid if `ServiceType == Schema`.
  std::string schema_name_;

  /// The catalog version. Must be valid if `ServiceType == Schema`.
  std::string version_;

  /// The pipeline ID. Valid if `HRNServiceType == Pipeline`. Must be valid if
  /// `ServiceType == Schema`.
  std::string pipeline_id_;
};

}  // namespace client
}  // namespace olp
