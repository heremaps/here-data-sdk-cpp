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

/// Deletes index data from an index layer.
class DATASERVICE_WRITE_API DeleteIndexDataRequest {
 public:
  /// A default constructor.
  DeleteIndexDataRequest() = default;

  /**
   * @return Layer ID previously set.
   */
  inline const std::string& GetLayerId() const { return layer_id_; }

  /**
   * @param layer_id The ID of the layer that the data blob belongs to.
   * Must be index layer.
   * @note Required.
   */
  inline DeleteIndexDataRequest& WithLayerId(const std::string& layer_id) {
    layer_id_ = layer_id;
    return *this;
  }

  /**
   * @param layer_id The ID of the layer that the data blob belongs to.
   * Must be index layer.
   * @note Required.
   */
  inline DeleteIndexDataRequest& WithLayerId(std::string&& layer_id) {
    layer_id_ = std::move(layer_id);
    return *this;
  }

  /**
   * @return Index ID previously set.
   */
  inline const std::string& GetIndexId() const { return index_id_; }

  /**
   * @param index_id Index ID which the data blob was associated with.
   * @note Required. It should be included in the callback of
   * IndexLayerClient::PublishIndex(const model::PublishIndexRequest& , const
   * PublishIndexCallback&) if operation was successful.
   */
  inline DeleteIndexDataRequest& WithIndexId(const std::string& index_id) {
    index_id_ = index_id;
    return *this;
  }

  /**
   * @param index_id Index ID which the data blob was associated with.
   * @note Required. It should be included in the callback of
   * IndexLayerClient::PublishIndex(const model::PublishIndexRequest& , const
   * PublishIndexCallback&) if operation was successful.
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
