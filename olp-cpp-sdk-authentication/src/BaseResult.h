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

#include <ctime>
#include <list>
#include <memory>
#include <string>

#include <rapidjson/document.h>

#include "olp/authentication/ErrorResponse.h"

namespace olp {
namespace authentication {
class BaseResult {
 public:
  BaseResult(int status, std::string error,
             std::shared_ptr<rapidjson::Document> json_document = nullptr);
  virtual ~BaseResult();

  /**
   * @brief Status getter method
   * @return status of HTTP response
   */
  int GetStatus() const;

  /**
   * @brief error string getter method
   * @return error string of failed request
   */
  const ErrorResponse& GetErrorResponse() const;

  /**
   * @brief List of all specific input field errors
   * @return List of input field errors
   */
  const ErrorFields& GetErrorFields() const;

  /**
   * @brief Gets a full error response message
   * @return A string representation of a full JSON network response if it
   * contains an error; an empty string otherwise
   */
  const std::string& GetFullMessage() const;

  /**
   * @brief has_error
   * @return True if response has an error status
   */
  bool HasError() const;

  /**
   * @brief is_valid
   * @return True if response is valid
   */
  virtual bool IsValid() const;

 private:
  bool is_valid_;

 protected:
  int status_;
  ErrorResponse error_;
  ErrorFields error_fields_;
  std::string full_message_;
};
}  // namespace authentication
}  // namespace olp
