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

#include "BaseResult.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include "Constants.h"
#include "olp/core/http/HttpStatusCode.h"

namespace olp {
namespace authentication {
static const char* FIELD_NAME = "name";
static const char* ERROR_CODE = "errorCode";
static const char* ERROR_FIELDS = "errorFields";
static const char* ERROR_ID = "errorId";
static const char* ERROR_MESSAGE = "message";
static const char* LINE_END = ".";

BaseResult::BaseResult(int status, std::string error,
                       std::shared_ptr<rapidjson::Document> json_document)
    : status_()

{
  status_ = status;
  error_.message = std::move(error);
  is_valid_ = (json_document && !json_document->HasParseError());

  // If HTTP error, try to get error details
  if (!HasError() || !is_valid_ || !json_document->HasMember(ERROR_CODE)) {
    return;
  }

  // The JSON document has an error code member, so save full JSON content to
  // the `full_message_` string.
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json_document->Accept(writer);
  full_message_ = buffer.GetString();

  if (json_document->HasMember(ERROR_ID)) {
    error_.error_id = (*json_document)[ERROR_ID].GetString();
  }

  // Enhance error message with network response error details
  error_.code = (*json_document)[ERROR_CODE].GetUint();

  if (!json_document->HasMember(ERROR_MESSAGE)) {
    return;
  }
  std::string message = (*json_document)[ERROR_MESSAGE].GetString();
  if (!json_document->HasMember(ERROR_FIELDS)) {
    error_.message = message;
    return;
  }

  error_.message = message.substr(0, message.find_first_of(LINE_END) + 1);
  const rapidjson::Value& fields = (*json_document)[ERROR_FIELDS];
  if (fields.GetType() == rapidjson::kArrayType) {
    for (rapidjson::SizeType i = 0u; i < fields.Size(); i++) {
      const rapidjson::Value& field = fields[i];
      if (field.HasMember(ERROR_MESSAGE)) {
        ErrorField error_field;
        error_field.name = field[FIELD_NAME].GetString();
        error_field.code = field[ERROR_CODE].GetUint();
        error_field.message = field[ERROR_MESSAGE].GetString();
        error_fields_.emplace_back(error_field);
      }
    }
  }
}

BaseResult::~BaseResult() = default;

int BaseResult::GetStatus() const { return status_; }

const ErrorResponse& BaseResult::GetErrorResponse() const { return error_; }

const ErrorFields& BaseResult::GetErrorFields() const { return error_fields_; }

const std::string& BaseResult::GetFullMessage() const { return full_message_; }

bool BaseResult::HasError() const {
  return status_ != http::HttpStatusCode::OK;
}

bool BaseResult::IsValid() const { return is_valid_; }

}  // namespace authentication
}  // namespace olp
