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

/// Checks whether data is present in a layer.
class DATASERVICE_WRITE_API CheckDataExistsRequest {
 public:
  CheckDataExistsRequest() = default;
  CheckDataExistsRequest(const CheckDataExistsRequest&) = default;
  CheckDataExistsRequest(CheckDataExistsRequest&&) = default;
  CheckDataExistsRequest& operator=(const CheckDataExistsRequest&) = default;
  CheckDataExistsRequest& operator=(CheckDataExistsRequest&&) = default;
  virtual ~CheckDataExistsRequest() = default;
  /**
   * @brief Gets the layer ID to which the data blob belongs.
   *
   * @return The layer ID.
   */
  inline const std::string& GetLayerId() const { return layer_id_; }

  /**
   * @brief Sets the layer ID to which the data blob belongs.
   *
   * @param layer_id The layer ID.
   */
  inline CheckDataExistsRequest& WithLayerId(const std::string& layer_id) {
    layer_id_ = layer_id;
    return *this;
  }

  /**
   * @brief Sets the layer ID to which the data blob belongs.
   *
   * @param layer_id The rvalue reference to the layer ID.
   */
  inline CheckDataExistsRequest& WithLayerId(std::string&& layer_id) {
    layer_id_ = std::move(layer_id);
    return *this;
  }

  /**
   * @brief Gets the data handle that is associated with the data blob.
   *
   * @return The data handle.
   */
  inline const std::string& GetDataHandle() const { return data_handle_; }

  /**
   * @brief Sets the data handle that is associated with the data blob.
   *
   * Include it in the callback of `VersionedLayerClient::PublishToBatch()`.
   * if the operation is successful.
   *
   * @param data_handle The data handle.
   */
  inline CheckDataExistsRequest& WithDataHandle(
      const std::string& data_handle) {
    data_handle_ = data_handle;
    return *this;
  }

  /**
   * @brief Sets the data handle that is associated with the data blob.
   *
   * Include it in the callback of `VersionedLayerClient::PublishToBatch()`.
   * if the operation is successful.
   *
   * @param data_handle The rvalue reference to the data handle.
   */
  inline CheckDataExistsRequest& WithDataHandle(
      const std::string&& data_handle) {
    data_handle_ = std::move(data_handle);
    return *this;
  }

 private:
  std::string layer_id_;
  std::string data_handle_;
};
}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
