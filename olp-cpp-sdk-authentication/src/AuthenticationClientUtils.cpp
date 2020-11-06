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

#include <chrono>
#include <iomanip>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "Constants.h"
#include <olp/authentication/Crypto.h>
#include "olp/core/http/NetworkUtils.h"
#include "olp/core/utils/Base64.h"
#include "olp/core/utils/Url.h"

namespace olp {
namespace authentication {

namespace {
// Helper characters
constexpr auto kParamAdd = "&";
constexpr auto kParamComma = ",";
constexpr auto kParamEquals = "=";
constexpr auto kParamQuote = "\"";
constexpr auto kLineFeed = '\n';

constexpr auto kOauthPost = "POST";
constexpr auto kOauthVersion = "oauth_version";
constexpr auto kOauthConsumerKey = "oauth_consumer_key";
constexpr auto kOauthNonce = "oauth_nonce";
constexpr auto kOauthSignature = "oauth_signature";
constexpr auto kOauthTimestamp = "oauth_timestamp";
constexpr auto kOauthSignatureMethod = "oauth_signature_method";
constexpr auto kVersion = "1.0";
constexpr auto kHmac = "HMAC-SHA256";

std::string Base64Encode(const Crypto::Sha256Digest& digest) {
  std::string ret = olp::utils::Base64Encode(digest.data(), digest.size());
  // Base64 encode sometimes return multiline with garbage at the end
  if (!ret.empty()) {
    auto loc = ret.find(kLineFeed);
    if (loc != std::string::npos) ret = ret.substr(0, loc);
  }
  return ret;
}

}  // namespace

namespace client = olp::client;

constexpr auto kDate = "date";

#ifdef _WIN32
#define timegm _mkgmtime
#endif

std::time_t ParseTime(const std::string& value) {
  std::tm tm = {};
  std::istringstream ss(value);
  ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S %z");
  return timegm(&tm);
}

std::time_t GetTimestampFromHeaders(const olp::http::Headers& headers) {
  auto it =
      std::find_if(begin(headers), end(headers),
                   [](const std::pair<std::string, std::string>& obg) {
                     return olp::http::NetworkUtils::CaseInsensitiveCompare(
                         obg.first, kDate);
                   });
  if (it != end(headers)) {
    return ParseTime(it->second);
  }
  return 0;
}

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

DecisionType GetDecision(const std::string& str) {
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
          GetDecision(element[Constants::DECISION].GetString()));
      // get permissions if avialible
      if (element.HasMember(Constants::PERMISSIONS) &&
          element[Constants::PERMISSIONS].IsArray()) {
        std::vector<Permission> permissions;
        const auto& permissions_array =
            element[Constants::PERMISSIONS].GetArray();
        for (auto& permission_element : permissions_array) {
          Permission permission;
          if (permission_element.HasMember(Constants::ACTION)) {
            permission.SetAction(
                permission_element[Constants::ACTION].GetString());
          }
          if (permission_element.HasMember(Constants::DECISION)) {
            permission.SetDecision(GetDecision(
                permission_element[Constants::DECISION].GetString()));
          }
          if (permission_element.HasMember(Constants::RESOURCE)) {
            permission.SetResource(
                permission_element[Constants::RESOURCE].GetString());
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
    result.SetDecision(GetDecision(doc[Constants::DECISION].GetString()));
  }

  // get diagnostics if available
  if (doc.HasMember(Constants::DIAGNOSTICS) &&
      doc[Constants::DIAGNOSTICS].IsArray()) {
    result.SetActionResults(GetDiagnostics(doc));
  }
  return result;
}

client::OlpClient CreateOlpClient(
    const AuthenticationSettings& auth_settings,
    boost::optional<client::AuthenticationSettings> authentication_settings) {
  client::OlpClientSettings settings;
  settings.network_request_handler = auth_settings.network_request_handler;
  settings.authentication_settings = authentication_settings;
  settings.proxy_settings = auth_settings.network_proxy_settings;
  settings.retry_settings.backdown_strategy =
      client::ExponentialBackdownStrategy();

  client::OlpClient client;
  client.SetBaseUrl(auth_settings.token_endpoint_url);
  client.SetSettings(std::move(settings));
  return client;
}

std::string GenerateAuthorizationHeader(
    const AuthenticationCredentials& credentials, const std::string& url,
    time_t timestamp, std::string nonce) {
  const std::string timestamp_str = std::to_string(timestamp);

  std::stringstream query;

  query << kOauthConsumerKey << kParamEquals << credentials.GetKey()
         << kParamAdd << kOauthNonce << kParamEquals << nonce << kParamAdd
         << kOauthSignatureMethod << kParamEquals << kHmac << kParamAdd
         << kOauthTimestamp << kParamEquals << timestamp_str << kParamAdd
         << kOauthVersion << kParamEquals << kVersion;

  const auto encoded_query = utils::Url::Encode(query.str());

  std::stringstream signature_base;

  signature_base << kOauthPost << kParamAdd << utils::Url::Encode(url) << kParamAdd
         << encoded_query;

  const std::string encode_key = credentials.GetSecret() + kParamAdd;
  auto hmac_result = Crypto::HmacSha256(encode_key, signature_base.str());
  auto signature = Base64Encode(hmac_result);

  std::stringstream authorization;

  authorization << "OAuth " << kOauthConsumerKey << kParamEquals << kParamQuote
                << utils::Url::Encode(credentials.GetKey()) << kParamQuote
                << kParamComma << kOauthNonce << kParamEquals << kParamQuote
                << utils::Url::Encode(nonce) << kParamQuote << kParamComma
                << kOauthSignatureMethod << kParamEquals << kParamQuote << kHmac
                << kParamQuote << kParamComma << kOauthTimestamp << kParamEquals
                << kParamQuote << utils::Url::Encode(timestamp_str)
                << kParamQuote << kParamComma << kOauthVersion << kParamEquals
                << kParamQuote << kVersion << kParamQuote << kParamComma
                << kOauthSignature << kParamEquals << kParamQuote
                << utils::Url::Encode(signature) << kParamQuote;

  return authorization.str();
}

}  // namespace authentication
}  // namespace olp
