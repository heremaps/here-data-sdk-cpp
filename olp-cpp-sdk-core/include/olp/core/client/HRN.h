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
   */
  explicit HRN(const std::string& input);

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

  HRN() = default;

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
   * @brief The partition of the HRN.
   *
   * Must be valid when `ServiceType == Data` or when `ServiceType == Pipeline`.
   *
   * @deprecated This field will be marked as private by 05.2020.
   */
  std::string partition;
  /**
   * @brief The service type of the HRN.
   *
   * @deprecated This field will be marked as private by 05.2020.
   */
  ServiceType service{ServiceType::Unknown};
  /**
   * @brief The region of the HRN.
   *
   * @deprecated This field will be marked as private by 05.2020.
   */
  std::string region;
  /**
   * @brief The account of the HRN.
   *
   * @deprecated This field will be marked as private by 05.2020.
   */
  std::string account;
  /**
   * @brief The catalog ID.
   *
   * Must be valid when `ServiceType == Data`.
   *
   * @deprecated This field will be marked as private by 05.2020.
   */
  std::string catalogId;
  /**
   * @brief (Optional) The layer ID.
   *
   * @deprecated This field will be marked as private by 05.2020.
   */
  std::string layerId;

  /**
   * @brief The group ID.
   *
   * Must be valid if `ServiceType == Schema`.
   *
   * @deprecated This field will be marked as private by 05.2020.
   */
  std::string groupId;
  /**
   * @brief The schema name.
   *
   * Must be valid if `ServiceType == Schema`.
   *
   * @deprecated This field will be marked as private by 05.2020.
   */
  std::string schemaName;
  /**
   * @brief The catalog version.
   *
   * Must be valid if `ServiceType == Schema`.
   *
   * @deprecated This field will be marked as private by 05.2020.
   */
  std::string version;

  /**
   * @brief The pipeline ID. Valid if `HRNServiceType == Pipeline`.
   *
   * Must be valid if `ServiceType == Schema`.
   *
   * @deprecated This field will be marked as private by 05.2020.
   */
  std::string pipelineId;
};

}  // namespace client
}  // namespace olp
