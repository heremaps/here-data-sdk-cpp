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
 * @brief The Here Resource Name (HRN) class, this allows HRN's to be passed to
 * the operations that require it.
 */
class CORE_API HRN {
 public:
  /**
   * @brief The ServiceType enum defines the different objects that a HRN can
   * refer to.
   */
  enum ServiceType {
    Unknown,  ///< The service type is unknown.
    Data,     ///< This HRN represents a data catalog.
    Schema,   ///< This HRN represents a schema type.
    Pipeline  ///< This HRN represents a pipeline instance.
  };

  /**
   * @brief Constructs a `HRN` from its string form.
   *
   * The passed string must start with "hrn:".
   *
   * @param input The `HRN` in string form.
   */
  explicit HRN(const std::string& input);

  /**
   * @brief Constructs a new instance of `HRN` from its string form.
   *
   * The passed string must start with "hrn:".
   *
   * @param input The `HRN` in string form.
   *
   * @return A new instance of `HRN` created from string.
   */
  static HRN FromString(const std::string& input);

  /**
   * @brief Constructs a unique instance of `HRN` from its string form.
   *
   * The passed string must start with "hrn:".
   *
   * @param input The `HRN` in string form.
   *
   * @return A new instance of `HRN` created from string.
   */
  static std::unique_ptr<HRN> UniqueFromString(const std::string& input);

  HRN() = default;

  /**
   * @brief The HRN class equality operator.
   */
  bool operator==(const HRN& rhs) const;

  /**
   * @brief The HRN class inequality operator.
   */
  bool operator!=(const HRN& rhs) const;

  /**
   * @brief The HRN class bool operator.
   *
   * @return Returns true if this `HRN` instance has all service type relevant
   * fields not empty, false otherwise.
   */
  explicit operator bool() const;

  /**
   * @brief Checks the internal status of the `HRN` instance.
   *
   * @return Returns true if this `HRN` instance has at least one of the service
   * type relevant fields empty, false otherwise.
   */
  bool IsNull() const;

  /**
   * @brief Returns this `HRN` instance in its string form, prefixed with
   * "hrn:".
   */
  std::string ToString() const;

  /**
   * @brief Returns this `HRN` instance in its string form for catalog ID,
   * prefixed with "hrn:".
   *
   * @note Only relevant for when the `HRN` has ServiceType == Data.
   */
  std::string ToCatalogHRNString() const;

  /// The partition of the HRN, must be valid when ServiceType == Data or when
  /// when ServiceType == Pipeline
  /// @deprecated This field will be marked as private by 05.2020.
  std::string partition;
  /// The service of the HRN.
  /// @deprecated This field will be marked as private by 05.2020.
  ServiceType service{ServiceType::Unknown};
  /// The region of the HRN.
  /// @deprecated This field will be marked as private by 05.2020.
  std::string region;
  /// The account of the HRN.
  /// @deprecated This field will be marked as private by 05.2020.
  std::string account;

  /// The catalog ID, must be valid when ServiceType == Data.
  /// @deprecated This field will be marked as private by 05.2020.
  std::string catalogId;
  /// The layer ID, optional.
  /// @deprecated This field will be marked as private by 05.2020.
  std::string layerId;

  /// The group ID, must be valid when ServiceType == Schema.
  /// @deprecated This field will be marked as private by 05.2020.
  std::string groupId;
  /// The schema name, must be valid when ServiceType == Schema.
  /// @deprecated This field will be marked as private by 05.2020.
  std::string schemaName;
  /// The version, must be valid when ServiceType == Schema.
  /// @deprecated This field will be marked as private by 05.2020.
  std::string version;

  /// Pipeline ID, valid when ServiceType == Pipeline.
  /// @deprecated This field will be marked as private by 05.2020.
  std::string pipelineId;
};

}  // namespace client
}  // namespace olp
