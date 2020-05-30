/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include "AuthenticationClientUtils.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <chrono>
#include <iomanip>
#include <sstream>

#include "Constants.h"

namespace olp {
namespace authentication {

void ExecuteOrSchedule(
    const std::shared_ptr<olp::thread::TaskScheduler>& task_scheduler,
    olp::thread::TaskScheduler::CallFuncType&& func) {
  if (!task_scheduler) {
    // User didn't specify a TaskScheduler, execute sync
    func();
    return;
  }

  // Schedule for async execution
  task_scheduler->ScheduleTask(std::move(func));
}

IntrospectAppResult GetIntrospectAppResult(const rapidjson::Document& doc) {
  IntrospectAppResult result;
  if (doc.HasMember(Constants::CLIENT_ID)) {
    result.SetClientId(doc[Constants::CLIENT_ID].GetString());
  }
  if (doc.HasMember(Constants::NAME)) {
    result.SetName(doc[Constants::NAME].GetString());
  }
  if (doc.HasMember(Constants::DESCRIPTION)) {
    result.SetDescription(doc[Constants::DESCRIPTION].GetString());
  }
  if (doc.HasMember(Constants::REDIRECT_URIS)) {
    auto uris = doc[Constants::REDIRECT_URIS].GetArray();
    std::vector<std::string> value_array;
    value_array.reserve(uris.Size());
    for (auto& value : uris) {
      value_array.push_back(value.GetString());
    }
    result.SetReditectUris(std::move(value_array));
  }
  if (doc.HasMember(Constants::ALLOWED_SCOPES)) {
    auto scopes = doc[Constants::ALLOWED_SCOPES].GetArray();
    std::vector<std::string> value_array;
    value_array.reserve(scopes.Size());
    for (auto& value : scopes) {
      value_array.push_back(value.GetString());
    }
    result.SetAllowedScopes(std::move(value_array));
  }
  if (doc.HasMember(Constants::TOKEN_ENDPOINT_AUTH_METHOD)) {
    result.SetTokenEndpointAuthMethod(
        doc[Constants::TOKEN_ENDPOINT_AUTH_METHOD].GetString());
  }
  if (doc.HasMember(Constants::TOKEN_ENDPOINT_AUTH_METHOD_REASON)) {
    result.SetTokenEndpointAuthMethodReason(
        doc[Constants::TOKEN_ENDPOINT_AUTH_METHOD_REASON].GetString());
  }
  if (doc.HasMember(Constants::DOB_REQUIRED)) {
    result.SetDobRequired(doc[Constants::DOB_REQUIRED].GetBool());
  }
  if (doc.HasMember(Constants::TOKEN_DURATION)) {
    result.SetTokenDuration(doc[Constants::TOKEN_DURATION].GetInt());
  }
  if (doc.HasMember(Constants::REFERRERS)) {
    auto uris = doc[Constants::REFERRERS].GetArray();
    std::vector<std::string> value_array;
    value_array.reserve(uris.Size());
    for (auto& value : uris) {
      value_array.push_back(value.GetString());
    }
    result.SetReferrers(std::move(value_array));
  }
  if (doc.HasMember(Constants::STATUS)) {
    result.SetStatus(doc[Constants::STATUS].GetString());
  }
  if (doc.HasMember(Constants::APP_CODE_ENABLED)) {
    result.SetAppCodeEnabled(doc[Constants::APP_CODE_ENABLED].GetBool());
  }
  if (doc.HasMember(Constants::CREATED_TIME)) {
    result.SetCreatedTime(doc[Constants::CREATED_TIME].GetInt64());
  }
  if (doc.HasMember(Constants::REALM)) {
    result.SetRealm(doc[Constants::REALM].GetString());
  }
  if (doc.HasMember(Constants::TYPE)) {
    result.SetType(doc[Constants::TYPE].GetString());
  }
  if (doc.HasMember(Constants::RESPONSE_TYPES)) {
    auto types = doc[Constants::RESPONSE_TYPES].GetArray();
    std::vector<std::string> value_array;
    value_array.reserve(types.Size());
    for (auto& value : types) {
      value_array.push_back(value.GetString());
    }
    result.SetResponseTypes(std::move(value_array));
  }
  if (doc.HasMember(Constants::TIER)) {
    result.SetTier(doc[Constants::TIER].GetString());
  }
  if (doc.HasMember(Constants::HRN)) {
    result.SetHrn(doc[Constants::HRN].GetString());
  }
  return result;
}

DecisionType GetPermission(const std::string& str) {
  return (str.compare("allow") == 0) ? DecisionType::kAllow
                                     : DecisionType::kDeny;
}

std::vector<ActionResult> GetDiagnostics(rapidjson::Document& doc) {
  std::vector<ActionResult> results;
  const auto& array = doc[Constants::DIAGNOSTICS].GetArray();
  for (auto& element : array) {
    ActionResult action;
    if (element.HasMember(Constants::DECISION)) {
      action.SetDecision(
          GetPermission(element[Constants::DECISION].GetString()));
      // get permissions if avialible
      if (element.HasMember(Constants::PERMISSIONS) &&
          element[Constants::PERMISSIONS].IsArray()) {
        std::vector<ActionResult::Permissions> permissions;
        const auto& permissions_array =
            element[Constants::PERMISSIONS].GetArray();
        for (auto& permission_element : permissions_array) {
          ActionResult::Permissions permission;
          if (permission_element.HasMember(Constants::ACTION)) {
            permission.first =
                permission_element[Constants::ACTION].GetString();
          }
          if (permission_element.HasMember(Constants::DECISION)) {
            permission.second = GetPermission(
                permission_element[Constants::DECISION].GetString());
          }
          permissions.push_back(std::move(permission));
        }

        action.SetPermissions(std::move(permissions));
      }
    }
    results.push_back(std::move(action));
  }
  return results;
}

AuthorizeResult GetAuthorizeResult(rapidjson::Document& doc) {
  AuthorizeResult result;

  if (doc.HasMember(Constants::IDENTITY)) {
    auto uris = doc[Constants::IDENTITY].GetObject();

    if (uris.HasMember(Constants::CLIENT_ID)) {
      result.SetClientId(uris[Constants::CLIENT_ID].GetString());
    } else if (uris.HasMember(Constants::USER_ID)) {
      result.SetClientId(uris[Constants::USER_ID].GetString());
    }
  }

  if (doc.HasMember(Constants::DECISION)) {
    result.SetDecision(GetPermission(doc[Constants::DECISION].GetString()));
  }

  // get diagnostics if available
  if (doc.HasMember(Constants::DIAGNOSTICS) &&
      doc[Constants::DIAGNOSTICS].IsArray()) {
    result.SetActionResults(GetDiagnostics(doc));
  }
  return result;
}

std::time_t ParseTime(const std::string& value) {
  std::tm tm = {};
  std::istringstream ss(value);
  ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S %z");
#ifdef _WIN32
  return _mkgmtime(&tm);
#else
  return timegm(&tm);
#endif
}

}  // namespace authentication
}  // namespace olp
