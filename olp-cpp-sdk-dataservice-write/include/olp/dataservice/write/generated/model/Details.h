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

/// Details of the publication.
class DATASERVICE_WRITE_API Details {
 public:
  Details() = default;
  Details(const Details&) = default;
  Details(Details&&) = default;
  Details& operator=(const Details&) = default;
  Details& operator=(Details&&) = default;
  virtual ~Details() = default;

 private:
  std::string state_;
  std::string message_;
  int64_t started_{0};
  int64_t modified_{0};
  int64_t expires_{0};

 public:
  /**
   * @brief Gets the state of the publication.
   *
   * @return The state of the publication.
   */
  const std::string& GetState() const { return state_; }

  /**
   * @brief Gets a mutable reference to the publication state.
   *
   * @return The mutable reference to the publication state.
   */
  std::string& GetMutableState() { return state_; }

  /**
   * @brief Sets the state of the publication.
   *
   * @param value The state of the publicztion.
   */
  void SetState(const std::string& value) { this->state_ = value; }

  /**
   * @brief Gets the publication description.
   *
   * @return The publication description.
   */
  const std::string& GetMessage() const { return message_; }

  /**
   * @brief Gets a mutable reference to the publication description.
   *
   * @return The mutable reference to the publication description.
   */
  std::string& GetMutableMessage() { return message_; }

  /**
   * @brief Sets the publication description.
   *
   * @param value The publication description.
   */
  void SetMessage(const std::string& value) { this->message_ = value; }

  /**
   * @brief Gets the publication start timestamp.
   *
   * @return The publication start timestamp.
   */
  const int64_t& GetStarted() const { return started_; }

  /**
   * @brief Gets a mutable reference to the publication start timestamp.
   *
   * @return The mutable reference to the publication start timestamp.
   */
  int64_t& GetMutableStarted() { return started_; }

  /**
   * @brief Sets the publication start timestamp.
   *
   * @param value The publication start timestamp.
   */
  void SetStarted(const int64_t& value) { this->started_ = value; }

  /**
   * @brief Gets the timestamp of when the publication was modified.
   *
   * @return The timestamp of when the publication was modified.
   */
  const int64_t& GetModified() const { return modified_; }

  /**
   * @brief Gets a mutable reference to the timestamp of when
   * the publication was modified.
   *
   * @return The mutable reference to the timestamp of when
   * the publication was modified.
   */
  int64_t& GetMutableModified() { return modified_; }

  /**
   * @brief Sets the timestamp of when the publication was modified.
   *
   * @param value The timestamp of when the publication was modified.
   */
  void SetModified(const int64_t& value) { this->modified_ = value; }

  /**
   * @brief Gets the publication expiration timestamp.
   *
   * @return The publication expiration timestamp.
   */
  const int64_t& GetExpires() const { return expires_; }

  /**
   * @brief Gets a mutable reference to the publication expiration timestamp.
   *
   * @return The mutable reference to the publication expiration timestamp.
   */
  int64_t& GetMutableExpires() { return expires_; }

  /**
   * @brief Sets the expiry time of the publication.
   *
   * @param value The expiry time of the publication.
   */
  void SetExpires(const int64_t& value) { this->expires_ = value; }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
