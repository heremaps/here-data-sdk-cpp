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

class DATASERVICE_WRITE_API Details {
 public:
  Details() = default;
  virtual ~Details() = default;

 private:
  std::string state_;
  std::string message_;
  int64_t started_{0};
  int64_t modified_{0};
  int64_t expires_{0};

 public:
  // TODO: This is specified as an enum in Open API spec. Update to enum class,
  // code generator should support this case.
  const std::string& GetState() const { return state_; }
  std::string& GetMutableState() { return state_; }
  void SetState(const std::string& value) { this->state_ = value; }

  const std::string& GetMessage() const { return message_; }
  std::string& GetMutableMessage() { return message_; }
  void SetMessage(const std::string& value) { this->message_ = value; }

  const int64_t& GetStarted() const { return started_; }
  int64_t& GetMutableStarted() { return started_; }
  void SetStarted(const int64_t& value) { this->started_ = value; }

  const int64_t& GetModified() const { return modified_; }
  int64_t& GetMutableModified() { return modified_; }
  void SetModified(const int64_t& value) { this->modified_ = value; }

  const int64_t& GetExpires() const { return expires_; }
  int64_t& GetMutableExpires() { return expires_; }
  void SetExpires(const int64_t& value) { this->expires_ = value; }
};

}  // namespace model
}  // namespace write
}  // namespace dataservice
}  // namespace olp
