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

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {

/// Deletes index data from an index layer.
class DATASERVICE_WRITE_API DeleteIndexDataRequest {
 public:
  /// A default constructor.
  DeleteIndexDataRequest() = default;
  DeleteIndexDataRequest(const DeleteIndexDataRequest&) = default;
  DeleteIndexDataRequest(DeleteIndexDataRequest&&) = default;
  DeleteIndexDataRequest& operator=(const DeleteIndexDataRequest&) = default;
  DeleteIndexDataRequest& operator=(DeleteIndexDataRequest&&) = default;
  virtual ~DeleteIndexDataRequest() = default;

  /**
   * @brief Gets the layer ID to which the data blob belongs.
   *
   * @return The layer ID.
   */
  inline const std::string& GetLayerId() const { return layer_id_; }

  /**
   * @brief Sets the layer ID to which the data blob belongs.
   *
   * Make sure the layer is of the index type.
   *
   * @param layer_id The layer ID.
   */
  inline DeleteIndexDataRequest& WithLayerId(const std::string& layer_id) {
    layer_id_ = layer_id;
    return *this;
  }

  /**
   * @brief Sets the layer ID to which the data blob belongs.
   *
   * Make sure the layer is of the index type.
   *
   * @param layer_id The rvalue reference to the layer ID.
   */
  inline DeleteIndexDataRequest& WithLayerId(std::string&& layer_id) {
    layer_id_ = std::move(layer_id);
    return *this;
  }

  /**
   * @brief Gets the index ID that is associated with the data blob.
   *
   * @return The index ID.
   */
  inline const std::string& GetIndexId() const { return index_id_; }

  /**
   * @brief Sets the index ID that is associated with the data blob.
   *
   * Include it in the callback of `IndexLayerClient::PublishIndex()`
   * if the operation is successful.
   *
   * @param index_id The index ID.
   */
  inline DeleteIndexDataRequest& WithIndexId(const std::string& index_id) {
    index_id_ = index_id;
    return *this;
  }

  /**
   * @brief Sets the index ID that is associated with the data blob.
   *
   * Include it in the callback of `IndexLayerClient::PublishIndex()`
   * if the operation is successful.
   *
   * @param index_id The rvalue reference to the index ID.
   */
  inline DeleteIndexDataRequest& WithIndexId(const std::string&& index_id) {
    index_id_ = std::move(index_id);
    return *this;
  }

 private:
  std::string layer_id_;
  std::string index_id_;
};
}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
