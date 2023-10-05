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
 * @brief Metadata version for the layer for a given catalog version.
 */
class LayerVersion {
 public:
  LayerVersion() = default;
  LayerVersion(const LayerVersion&) = default;
  LayerVersion(LayerVersion&&) = default;
  LayerVersion& operator=(const LayerVersion&) = default;
  LayerVersion& operator=(LayerVersion&&) = default;
  virtual ~LayerVersion() = default;

 private:
  std::string layer_;
  int64_t version_{0};
  int64_t timestamp_{0};

 public:
  const std::string& GetLayer() const { return layer_; }
  std::string& GetMutableLayer() { return layer_; }
  void SetLayer(const std::string& value) { this->layer_ = value; }

  const int64_t& GetVersion() const { return version_; }
  int64_t& GetMutableVersion() { return version_; }
  void SetVersion(const int64_t& value) { this->version_ = value; }

  const int64_t& GetTimestamp() const { return timestamp_; }
  int64_t& GetMutableTimestamp() { return timestamp_; }
  void SetTimestamp(const int64_t& value) { this->timestamp_ = value; }
};

/**
 * @brief Describes the list of the layer versions for a given catalog version.
 */
class LayerVersions {
 public:
  LayerVersions() = default;
  LayerVersions(const LayerVersions&) = default;
  LayerVersions(LayerVersions&&) = default;
  LayerVersions& operator=(const LayerVersions&) = default;
  LayerVersions& operator=(LayerVersions&&) = default;
  virtual ~LayerVersions() = default;

 private:
  std::vector<LayerVersion> layer_versions_;
  int64_t version_{0};

 public:
  const std::vector<LayerVersion>& GetLayerVersions() const {
    return layer_versions_;
  }
  std::vector<LayerVersion>& GetMutableLayerVersions() {
    return layer_versions_;
  }
  void SetLayerVersions(const std::vector<LayerVersion>& value) {
    this->layer_versions_ = value;
  }

  /**
   * @brief Get the catalog version.
   */
  const int64_t& GetVersion() const { return version_; }
  int64_t& GetMutableVersion() { return version_; }
  void SetVersion(const int64_t& value) { this->version_ = value; }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
