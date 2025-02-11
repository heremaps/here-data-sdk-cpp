/*
 * Copyright (C) 2020-2025 HERE Europe B.V.
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

#include <olp/authentication/Crypto.h>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>
#include "Constants.h"
#include "ResponseFromJsonBuilder.h"
#include "olp/core/http/NetworkResponse.h"
#include "olp/core/http/NetworkUtils.h"
#include "olp/core/logging/Log.h"
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
constexpr auto kLogTag = "AuthenticationClientUtils";

std::string Base64Encode(const Crypto::Sha256Digest& digest) {
  std::string ret = olp::utils::Base64Encode(digest.data(), digest.size());
  // Base64 encode sometimes return multiline with garbage at the end
  if (!ret.empty()) {
    auto loc = ret.find(kLineFeed);
    if (loc != std::string::npos) {
      ret = ret.substr(0, loc);
    }
  }
  return ret;
}

Response<boost::json::object> Parse(client::HttpResponse& http_response) {
  boost::json::error_code ec;
  auto document = boost::json::parse(http_response.GetRawResponse(), ec);
  if (http_response.GetStatus() != http::HttpStatusCode::OK) {
    // HttpResult response can be error message or valid json with it.
    std::string msg = http_response.GetResponseAsString();
    if (!ec.failed() && document.is_object() &&
        document.as_object().contains(Constants::MESSAGE)) {
      msg = document.as_object()[Constants::MESSAGE].get_string().c_str();
    }
    return client::ApiError({http_response.GetStatus(), msg});
  }

  if (ec.failed() || !document.is_object()) {
    return client::ApiError({static_cast<int>(http::ErrorCode::UNKNOWN_ERROR),
                             "Failed to parse response"});
  }

  return {std::move(document).as_object()};
}
}  // namespace

namespace client = olp::client;

constexpr auto kDate = "date";

#ifdef _WIN32
// Windows does not have ::strptime and ::timegm
std::time_t ParseTime(const std::string& value) {
  std::tm tm = {};
  std::istringstream ss(value);
  ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S %z");
  return _mkgmtime(&tm);
}

#else

std::time_t ParseTime(const std::string& value) {
  std::tm tm = {};
  const auto format = "%a, %d %b %Y %H:%M:%S %Z";
  const auto parsed_until = ::strptime(value.c_str(), format, &tm);
  if (parsed_until != value.c_str() + value.size()) {
    OLP_SDK_LOG_WARNING(kLogTag, "Timestamp is not fully parsed" << value);
  }
  // MacOS updates `tm_isdst`, `tm_zone` and `tm_gmtoff` fields in `timegm`
  // call.
  const auto gmtoff = tm.tm_gmtoff;
  const auto local_time = timegm(&tm);
  return local_time - gmtoff;
}

#endif

boost::optional<std::time_t> GetTimestampFromHeaders(
    const olp::http::Headers& headers) {
  auto it =
      std::find_if(begin(headers), end(headers),
                   [](const std::pair<std::string, std::string>& obg) {
                     return olp::http::NetworkUtils::CaseInsensitiveCompare(
                         obg.first, kDate);
                   });
  if (it != end(headers)) {
    return ParseTime(it->second);
  }
  return boost::none;
}

IntrospectAppResult GetIntrospectAppResult(const boost::json::object& doc) {
  return ResponseFromJsonBuilder<IntrospectAppResult>::Build(doc)
      .Value(Constants::CLIENT_ID, &IntrospectAppResult::SetClientId)
      .Value(Constants::NAME, &IntrospectAppResult::SetName)
      .Value(Constants::DESCRIPTION, &IntrospectAppResult::SetDescription)
      .Array(Constants::REDIRECT_URIS, &IntrospectAppResult::SetReditectUris)
      .Array(Constants::ALLOWED_SCOPES, &IntrospectAppResult::SetAllowedScopes)
      .Value(Constants::TOKEN_ENDPOINT_AUTH_METHOD,
             &IntrospectAppResult::SetTokenEndpointAuthMethod)
      .Value(Constants::TOKEN_ENDPOINT_AUTH_METHOD_REASON,
             &IntrospectAppResult::SetTokenEndpointAuthMethodReason)
      .Value(Constants::DOB_REQUIRED, &IntrospectAppResult::SetDobRequired)
      .Value(Constants::TOKEN_DURATION, &IntrospectAppResult::SetTokenDuration)
      .Array(Constants::REFERRERS, &IntrospectAppResult::SetReferrers)
      .Value(Constants::STATUS, &IntrospectAppResult::SetStatus)
      .Value(Constants::APP_CODE_ENABLED,
             &IntrospectAppResult::SetAppCodeEnabled)
      .Value(Constants::CREATED_TIME, &IntrospectAppResult::SetCreatedTime)
      .Value(Constants::REALM, &IntrospectAppResult::SetRealm)
      .Value(Constants::TYPE, &IntrospectAppResult::SetType)
      .Array(Constants::RESPONSE_TYPES, &IntrospectAppResult::SetResponseTypes)
      .Value(Constants::TIER, &IntrospectAppResult::SetTier)
      .Value(Constants::HRN, &IntrospectAppResult::SetHrn)
      .Finish();
}

DecisionType GetDecision(const std::string& str) {
  return (str.compare("allow") == 0) ? DecisionType::kAllow
                                     : DecisionType::kDeny;
}

std::vector<ActionResult> GetDiagnostics(boost::json::object& doc) {
  std::vector<ActionResult> results;
  auto& array = doc[Constants::DIAGNOSTICS].get_array();
  for (auto& element_value : array) {
    auto& element = element_value.get_object();
    ActionResult action;
    if (element.contains(Constants::DECISION)) {
      action.SetDecision(
          GetDecision(element[Constants::DECISION].get_string().c_str()));
      // get permissions if avialible
      if (element.contains(Constants::PERMISSIONS) &&
          element[Constants::PERMISSIONS].is_array()) {
        std::vector<Permission> permissions;
        const auto& permissions_array =
            element[Constants::PERMISSIONS].get_array();
        for (auto& permission_element : permissions_array) {
          Permission permission =
              ResponseFromJsonBuilder<Permission>::Build(
                  permission_element.get_object())
                  .Value(Constants::ACTION, &Permission::SetAction)
                  .Value(Constants::DECISION, &Permission::SetDecision,
                         &GetDecision)
                  .Value(Constants::RESOURCE, &Permission::SetResource)
                  .Finish();
          permissions.push_back(std::move(permission));
        }

        action.SetPermissions(std::move(permissions));
      }
    }
    results.push_back(std::move(action));
  }
  return results;
}

AuthorizeResult GetAuthorizeResult(boost::json::object& doc) {
  AuthorizeResult result;

  if (doc.contains(Constants::IDENTITY)) {
    auto uris = doc[Constants::IDENTITY].get_object();

    if (uris.contains(Constants::CLIENT_ID)) {
      result.SetClientId(uris[Constants::CLIENT_ID].get_string().c_str());
    } else if (uris.contains(Constants::USER_ID)) {
      result.SetClientId(uris[Constants::USER_ID].get_string().c_str());
    }
  }

  if (doc.contains(Constants::DECISION)) {
    result.SetDecision(
        GetDecision(doc[Constants::DECISION].get_string().c_str()));
  }

  // get diagnostics if available
  if (doc.contains(Constants::DIAGNOSTICS) &&
      doc[Constants::DIAGNOSTICS].is_array()) {
    result.SetActionResults(GetDiagnostics(doc));
  }
  return result;
}

UserAccountInfoResponse GetUserAccountInfoResponse(
    client::HttpResponse& http_response) {
  using UserAccountInfo = model::UserAccountInfoResponse;

  const auto parse_response = Parse(http_response);
  if (!parse_response.IsSuccessful()) {
    return parse_response.GetError();
  }

  const auto& document = parse_response.GetResult();

  return ResponseFromJsonBuilder<UserAccountInfo>::Build(document)
      .Value(Constants::USER_ID, &UserAccountInfo::SetUserId)
      .Value(Constants::REALM, &UserAccountInfo::SetRealm)
      .Value(Constants::FACEBOOK_ID, &UserAccountInfo::SetFacebookId)
      .Value(Constants::FIRSTNAME, &UserAccountInfo::SetFirstname)
      .Value(Constants::LASTNAME, &UserAccountInfo::SetLastname)
      .Value(Constants::EMAIL, &UserAccountInfo::SetEmail)
      .Value(Constants::RECOVERY_EMAIL, &UserAccountInfo::SetRecoveryEmail)
      .Value(Constants::DOB, &UserAccountInfo::SetDob)
      .Value(Constants::COUNTRY_CODE, &UserAccountInfo::SetCountryCode)
      .Value(Constants::LANGUAGE, &UserAccountInfo::SetLanguage)
      .Value(Constants::EMAIL_VERIFIED, &UserAccountInfo::SetEmailVerified)
      .Value(Constants::PHONE_NUMBER, &UserAccountInfo::SetPhoneNumber)
      .Value(Constants::PHONE_NUMBER_VERIFIED,
             &UserAccountInfo::SetPhoneNumberVerified)
      .Value(Constants::MARKETING_ENABLED,
             &UserAccountInfo::SetMarketingEnabled)
      .Value(Constants::CREATED_TIME, &UserAccountInfo::SetCreatedTime)
      .Value(Constants::UPDATED_TIME, &UserAccountInfo::SetUpdatedTime)
      .Value(Constants::STATE, &UserAccountInfo::SetState)
      .Value(Constants::HRN, &UserAccountInfo::SetHrn)
      .Value(Constants::ACCOUNT_TYPE, &UserAccountInfo::SetAccountType)
      .Finish();
}

client::OlpClient CreateOlpClient(
    const AuthenticationSettings& auth_settings,
    boost::optional<client::AuthenticationSettings> authentication_settings,
    bool retry) {
  client::OlpClientSettings settings;
  settings.network_request_handler = auth_settings.network_request_handler;
  settings.authentication_settings = authentication_settings;
  settings.proxy_settings = auth_settings.network_proxy_settings;
  settings.retry_settings = auth_settings.retry_settings;

  if (!retry) {
    settings.retry_settings.max_attempts = 0;
  }

  client::OlpClient client(settings, auth_settings.token_endpoint_url);
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

  signature_base << kOauthPost << kParamAdd << utils::Url::Encode(url)
                 << kParamAdd << encoded_query;

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
