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
#include <utility>
#include <vector>

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {

/// Checks whether the data is present in a layer.
class DATASERVICE_WRITE_API CheckDataExistsRequest {
 public:
  CheckDataExistsRequest() = default;

  /**
   * @return Layer ID previously set.
   */
  inline const std::string& GetLayerId() const { return layer_id_; }

  /**
   * @param layer_id The ID of the layer that the data blob belongs to.
   * @note Required.
   */
  inline CheckDataExistsRequest& WithLayerId(const std::string& layer_id) {
    layer_id_ = layer_id;
    return *this;
  }

  /**
   * @param layer_id The ID of the layer that the data blob belongs to.
   * @note Required.
   */
  inline CheckDataExistsRequest& WithLayerId(std::string&& layer_id) {
    layer_id_ = std::move(layer_id);
    return *this;
  }

  /**
   * @return DataHandle previously set.
   */
  inline const std::string& GetDataHandle() const { return data_handle_; }

  /**
   * @param data_handle Data Handle which the data blob was associated with.
   * @note Required. It should be included in the callback of
   * VersionedLayerClient::PublishToBatch(
   const model::Publication& pub,
   const model::PublishPartitionDataRequest& request) if operation was
   successful.
   */
  inline CheckDataExistsRequest& WithDataHandle(
      const std::string& data_handle) {
    data_handle_ = data_handle;
    return *this;
  }

  /**
   * @param data_handle Index ID which the data blob was associated with.
   * @note Required. It should be included in the callback of
   * VersionedLayerClient::PublishToBatch(
   const model::Publication& pub,
   const model::PublishPartitionDataRequest& request) if operation was
   successful.
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
