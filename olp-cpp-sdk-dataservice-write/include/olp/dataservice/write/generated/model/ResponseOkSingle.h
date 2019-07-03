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

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {
/**
 * @brief Model to represent response for successful data upload to catalog
 * layer.
 */
class DATASERVICE_WRITE_API ResponseOkSingle {
 public:
  ResponseOkSingle() = default;
  virtual ~ResponseOkSingle() = default;

 private:
  std::string trace_id_;

 public:
  /**
   * @brief Get the partition id where the handle to the data can be found.
   */
  const std::string& GetTraceID() const { return trace_id_; }

  /**
   * @brief Get the partition id where the handle to the data can be found.
   */
  std::string& GetMutableTraceID() { return trace_id_; }

  /**
   * @brief Set the partition id where the handle to the data can be found.
   */
  void SetTraceID(const std::string& value) { this->trace_id_ = value; }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
