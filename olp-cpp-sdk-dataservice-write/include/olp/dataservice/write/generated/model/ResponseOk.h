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
 * @brief Contains IDs that can be used to track your request and identify
 * the messages in the catalog.
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
   * @brief Gets the unique ID of the list of messages.
   *
   * You can use this ID to track your request and identify
   * each message in the catalog.
   *
   * @return The unique ID of the list of messages.
   */
  const std::string& GetParentID() const { return parent_id_; }

  /**
   * @brief Gets a mutable reference to the unique ID of the list of messages.
   *
   * You can use this ID to track your request and identify
   * the message in the catalog.
   *
   * @return The mutable reference to the unique ID of the list of messages.
   */
  std::string& GetMutableParentID() { return parent_id_; }

  /**
   * @brief Sets the unique ID for the list of messages.
   *
   * You can use this ID to track your request and identify
   * the message in the catalog.
   *
   * @param value The unique ID to set.
   */
  void SetParentID(const std::string& value) { this->parent_id_ = value; }

  /**
   * @brief Gets the generated list of unique message IDs.
   *
   * You can use this ID to track your request and identify the message in the catalog.
   *
   * @return The generated list of unique message IDs.
   */
  const std::vector<std::string>& GetGeneratedIDs() const {
    return generated_ids_;
  }

  /**
   * @brief Gets a mutable reference to the generated list of unique message IDs.
   *
   * You can use this ID to track your request and identify the message in the catalog.
   *
   * @return The mutable reference to the generated list of unique message IDs.
   */
  std::vector<std::string>& GetMutableGeneratedIDs() { return generated_ids_; }

  /**
   * @brief Sets the generated list of unique message IDs.
   *
   * @param value The generated list of unique message IDs.
   */
  void SetGeneratedIDs(const std::vector<std::string>& value) {
    this->generated_ids_ = value;
  }
};

/// Contians response data for a successful ingestSDII call.
class DATASERVICE_WRITE_API ResponseOk {
 public:
  ResponseOk() = default;
  virtual ~ResponseOk() = default;

 private:
  TraceID trace_id_;

 public:
  /**
   * @brief Gets the IDs that can be used to track your request and identify
   * the messages in the catalog.
   *
   * @return The trace IDs.
   */
  const TraceID& GetTraceID() const { return trace_id_; }

  /**
   * @brief Gets a mutable reference to the IDs that can be used to track
   * your request and identify the messages in the catalog.
   *
   * @return The mutable reference to the trace IDs.
   */
  TraceID& GetMutableTraceID() { return trace_id_; }

  /**
   * @brief Sets the IDs that can be used to track your request and identify
   * the messages in the catalog.
   *
   * @param value The trace IDs.
   */
  void SetTraceID(const TraceID& value) { this->trace_id_ = value; }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
