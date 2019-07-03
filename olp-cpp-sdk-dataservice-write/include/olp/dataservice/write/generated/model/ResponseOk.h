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
#include <vector>

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {
/**
 * @brief Contains IDs which can be used to track your request and identify the
 * messages in the catalog.
 */
class DATASERVICE_WRITE_API TraceID {
 public:
  TraceID() = default;
  virtual ~TraceID() = default;

 private:
  std::string parent_id_;
  std::vector<std::string> generated_ids_;

 public:
  /**
   * @brief A unique ID for the messages list. You can use this ID to track your
   * request and identify the message in the catalog.
   */
  const std::string& GetParentID() const { return parent_id_; }

  /**
   * @brief A unique ID for the messages list. You can use this ID to track your
   * request and identify the message in the catalog.
   */
  std::string& GetMutableParentID() { return parent_id_; }

  void SetParentID(const std::string& value) { this->parent_id_ = value; }

  /**
   * @brief Generated list of unique IDs for each message in a list. You can use
   * this ID to track your request and identify the message in the catalog.
   */
  const std::vector<std::string>& GetGeneratedIDs() const {
    return generated_ids_;
  }

  /**
   * @brief Generated list of unique IDs for each message in a list. You can use
   * this ID to track your request and identify the message in the catalog.
   */
  std::vector<std::string>& GetMutableGeneratedIDs() { return generated_ids_; }

  void SetGeneratedIDs(const std::vector<std::string>& value) {
    this->generated_ids_ = value;
  }
};

/**
 * @brief Contians response data for a successful ingestSDII call.
 */
class DATASERVICE_WRITE_API ResponseOk {
 public:
  ResponseOk() = default;
  virtual ~ResponseOk() = default;

 private:
  TraceID trace_id_;

 public:
  /**
   * @brief Contains IDs which can be used to track your request and identify
   * the messages in the catalog.
   */
  const TraceID& GetTraceID() const { return trace_id_; }

  /**
   * @brief Contains IDs which can be used to track your request and identify
   * the messages in the catalog.
   */
  TraceID& GetMutableTraceID() { return trace_id_; }

  void SetTraceID(const TraceID& value) { this->trace_id_ = value; }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
