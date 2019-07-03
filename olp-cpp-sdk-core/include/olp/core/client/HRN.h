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
/// \brief The Here Resource Name (HRN) class, this allows HRN's to be passed to
/// the operations that require it.
class CORE_API HRN {
 public:
  /// The ServiceType enum defines the different objects that a HRN can refer to
  enum ServiceType {
    Unknown,  ///< the service type is unknown
    Data,     ///< a data catalog
    Schema,   ///< a schema type
    Pipeline  ///< a pipeline instance
  };

  /// the partition of the HRN
  std::string partition;
  /// the service of the HRN
  ServiceType service{ServiceType::Unknown};
  /// the region of the HRN
  std::string region;
  /// the account of the HRN
  std::string account;

  /// Catalog ID, valid when HRNServiceType == Data
  std::string catalogId;
  /// Layer ID, optional
  std::string layerId;

  /// Group ID, valid when HRNServiceType == Schema
  std::string groupId;
  /// Schema Name
  std::string schemaName;
  /// Version
  std::string version;

  /// Pipeline ID, valid when HRNServiceType == Pipeline
  std::string pipelineId;

  /// \brief returns this HRN in its string form, prefixed with hrn:
  std::string ToString() const;

  /// \brief returns this HRN in its string form for catalog ID, prefixed with
  /// hrn:
  std::string ToCatalogHRNString() const;

  /// \brief default constructor
  HRN() = default;

  /// \brief constructs a HRN from its string form. Must start with "hrn:".
  /// As a special case to support existing command line clients, http: and
  /// https: are also accepted
  explicit HRN(const std::string& input);

  /// \brief constructs a HRN from its string form. Must start with "hrn:".
  /// As a special case to support existing command line clients, http: and
  /// https: are also accepted
  static HRN FromString(const std::string& input);

  /// \brief constructs a HRN from its string form. Must start with "hrn:".
  /// As a special case to support existing command line clients, http: and
  /// https: are also accepted
  static std::unique_ptr<HRN> UniqueFromString(const std::string& input);

  /// \brief The HRN class equality operator
  bool operator==(const HRN& other) const;

  /// \brief The HRN class inequality operator
  bool operator!=(const HRN& other) const;

  /// \brief returns true if HRN is null (all fields empty), false otherwise
  bool IsNull() const;
};
}  // namespace client
}  // namespace olp
