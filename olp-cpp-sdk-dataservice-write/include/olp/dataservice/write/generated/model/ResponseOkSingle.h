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

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {
/**
 * @brief Represents a response to a successful data upload operation
 * to a catalog layer.
 */
class DATASERVICE_WRITE_API ResponseOkSingle {
 public:
  ResponseOkSingle() = default;
  ResponseOkSingle(const ResponseOkSingle&) = default;
  ResponseOkSingle(ResponseOkSingle&&) = default;
  ResponseOkSingle& operator=(const ResponseOkSingle&) = default;
  ResponseOkSingle& operator=(ResponseOkSingle&&) = default;
  virtual ~ResponseOkSingle() = default;

 private:
  std::string trace_id_;

 public:
  /**
   * @brief Gets the trace ID of the request.
   *
   * It is a unique message ID, such as a UUID.
   * You can use this ID to track your request and identify the
   * message in the catalog.
   *
   * @return The trace ID of the request.
   */
  const std::string& GetTraceID() const { return trace_id_; }

  /**
   * @brief Gets a mutable reference to the trace ID of the request.
   *
   * @see `GetTraceID` for more information on the trace ID.
   *
   * @return The trace ID of the request.
   */
  std::string& GetMutableTraceID() { return trace_id_; }

  /**
   * @brief Sets the trace ID of the request.
   *
   * @param value A unique message ID, such as a UUID. If you want to
   * define your ID, include it in the request. If you do not
   * include an ID, it is generated during ingestion and included in
   * the response. You can use this ID to track your request and identify
   * the message in the catalog.
   */
  void SetTraceID(const std::string& value) { this->trace_id_ = value; }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
