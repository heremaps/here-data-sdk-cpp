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

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {
namespace model {
/**
 * @brief Contains IDs that can be used to track your request and identify
 * messages in a catalog.
 */
class DATASERVICE_WRITE_API TraceID {
 public:
  TraceID() = default;
  TraceID(const TraceID&) = default;
  TraceID(TraceID&&) = default;
  TraceID& operator=(const TraceID&) = default;
  TraceID& operator=(TraceID&&) = default;
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
   * You can use this ID to track your request and identify the message in the
   * catalog.
   *
   * @return The generated list of unique message IDs.
   */
  const std::vector<std::string>& GetGeneratedIDs() const {
    return generated_ids_;
  }

  /**
   * @brief Gets a mutable reference to the generated list of unique message
   * IDs.
   *
   * You can use this ID to track your request and identify the message in the
   * catalog.
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

/// Contians a response to a successful ingestSDII call.
class DATASERVICE_WRITE_API ResponseOk {
 public:
  ResponseOk() = default;
  ResponseOk(const ResponseOk&) = default;
  ResponseOk(ResponseOk&&) = default;
  ResponseOk& operator=(const ResponseOk&) = default;
  ResponseOk& operator=(ResponseOk&&) = default;
  virtual ~ResponseOk() = default;

 private:
  TraceID trace_id_;

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
  const TraceID& GetTraceID() const { return trace_id_; }

  /**
   * @brief Gets a mutable reference to the trace ID of the request.
   *
   * @see `GetTraceID` for more information on the trace ID.
   *
   * @return The trace ID of the request.
   */
  TraceID& GetMutableTraceID() { return trace_id_; }

  /**
   * @brief Sets the trace ID of the request.
   *
   * @param value A unique message ID, such as a UUID. If you want to
   * define your ID, include it in the request. If you do not
   * include an ID, it is generated during ingestion and included in
   * the response. You can use this ID to track your request and identify
   * the message in the catalog.
   */
  void SetTraceID(const TraceID& value) { this->trace_id_ = value; }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
