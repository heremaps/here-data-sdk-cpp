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

#include "AuthenticationClientImpl.h"

#include <chrono>
#include <iomanip>
#include <limits>
#include <sstream>
#include <thread>
#include <utility>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "AuthenticationClientUtils.h"
#include "Constants.h"
#include "SignInResultImpl.h"
#include "SignInUserResultImpl.h"
#include "SignOutResultImpl.h"
#include "SignUpResultImpl.h"
#include "olp/core/http/Network.h"
#include "olp/core/http/NetworkConstants.h"
#include "olp/core/http/NetworkResponse.h"
#include "olp/core/http/NetworkUtils.h"
#include "olp/core/logging/Log.h"
#include "olp/core/thread/TaskScheduler.h"
#include "olp/core/utils/Url.h"

namespace olp {
namespace authentication {
namespace {

using RequestBodyData = client::OlpClient::RequestBodyType::element_type;

// Tags
const std::string kOauthEndpoint = "/oauth2/token";
const std::string kSignoutEndpoint = "/logout";
const std::string kTermsEndpoint = "/terms";
const std::string kUserEndpoint = "/user";
const std::string kMyAccountEndpoint = "/user/me";
constexpr auto kTimestampEndpoint = "/timestamp";
constexpr auto kIntrospectAppEndpoint = "/app/me";
constexpr auto kDecisionEndpoint = "/decision/authorize";

// JSON fields
constexpr auto kCountryCode = "countryCode";
constexpr auto kDateOfBirth = "dob";
constexpr auto kEmail = "email";
constexpr auto kFirstName = "firstname";
constexpr auto kGrantType = "grantType";
constexpr auto kScope = "scope";
constexpr auto kDeviceId = "deviceId";
constexpr auto kInviteToken = "inviteToken";
constexpr auto kLanguage = "language";
constexpr auto kLastName = "lastname";
constexpr auto kMarketingEnabled = "marketingEnabled";
constexpr auto kPassword = "password";
constexpr auto kPhoneNumber = "phoneNumber";
constexpr auto kRealm = "realm";
constexpr auto kTermsReacceptanceToken = "termsReacceptanceToken";
constexpr auto kClientId = "clientId";
constexpr auto kGivenName = "givenName";
constexpr auto kFamilyName = "familyName";

constexpr auto kClientGrantType = "client_credentials";
constexpr auto kUserGrantType = "password";
constexpr auto kFacebookGrantType = "facebook";
constexpr auto kArcgisGrantType = "arcgis";
constexpr auto kAppleGrantType = "jwtIssNotHERE";
constexpr auto kRefreshGrantType = "refresh_token";

constexpr auto kServiceId = "serviceId";
constexpr auto kActions = "actions";
constexpr auto kAction = "action";
constexpr auto kResource = "resource";
constexpr auto kDiagnostics = "diagnostics";
constexpr auto kOperator = "operator";
// Values
constexpr auto kErrorWrongTimestamp = 401204;
constexpr auto kLogTag = "AuthenticationClient";
const auto kMaxTime = std::numeric_limits<time_t>::max();

bool HasWrongTimestamp(const SignInResult& result) {
  const auto& error_response = result.GetErrorResponse();
  const auto status = result.GetStatus();
  return status == http::HttpStatusCode::UNAUTHORIZED &&
         error_response.code == kErrorWrongTimestamp;
}

void RetryDelay(const client::RetrySettings& retry_settings, size_t retry) {
  std::this_thread::sleep_for(retry_settings.backdown_strategy(
      std::chrono::milliseconds(retry_settings.initial_backdown_period),
      retry));
}

client::OlpClient::RequestBodyType GenerateAppleSignInBody(
    const AppleSignInProperties& sign_in_properties) {
  rapidjson::StringBuffer data;
  rapidjson::Writer<rapidjson::StringBuffer> writer(data);
  writer.StartObject();

  writer.Key(kGrantType);
  writer.String(kAppleGrantType);

  auto write_field = [&writer](const char* key, const std::string& value) {
    if (!value.empty()) {
      writer.Key(key);
      writer.String(value.c_str());
    }
  };

  write_field(kClientId, sign_in_properties.GetClientId());
  write_field(kRealm, sign_in_properties.GetRealm());
  write_field(kGivenName, sign_in_properties.GetFirstname());
  write_field(kFamilyName, sign_in_properties.GetLastname());
  write_field(kCountryCode, sign_in_properties.GetCountryCode());
  write_field(kLanguage, sign_in_properties.GetLanguage());

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<RequestBodyData>(content, content + data.GetSize());
}

client::HttpResponse CallApi(const client::OlpClient& client,
                             const std::string& endpoint,
                             client::CancellationContext context,
                             const std::string& auth_header,
                             client::OlpClient::RequestBodyType body) {
  client::OlpClient::ParametersType headers{
      {http::kAuthorizationHeader, auth_header}};

  return client.CallApi(
      endpoint, "POST", {}, std::move(headers), {}, std::move(body),
      AuthenticationClientImpl::kApplicationJson, std::move(context));
}

std::string DeduceContentType(const SignInProperties& properties) {
  if (properties.custom_body) {
    return "";
  }
  return AuthenticationClientImpl::kApplicationJson;
}

}  // namespace

AuthenticationClientImpl::RequestTimer::RequestTimer()
    : timer_start_{std::chrono::steady_clock::now()},
      time_{std::chrono::system_clock::to_time_t(
          std::chrono::system_clock::now())} {}

AuthenticationClientImpl::RequestTimer::RequestTimer(std::time_t server_time)
    : timer_start_{std::chrono::steady_clock::now()}, time_{server_time} {}

std::time_t AuthenticationClientImpl::RequestTimer::GetRequestTime() const {
  const auto now = std::chrono::steady_clock::now();
  const auto time_since_start =
      std::chrono::duration_cast<std::chrono::seconds>(now - timer_start_);
  return time_ + time_since_start.count();
}

AuthenticationClientImpl::AuthenticationClientImpl(
    AuthenticationSettings settings)
    : client_token_cache_(
          std::make_shared<SignInCacheType>(settings.token_cache_limit)),
      user_token_cache_(
          std::make_shared<SignInUserCacheType>(settings.token_cache_limit)),
      settings_(std::move(settings)),
      pending_requests_(std::make_shared<client::PendingRequests>()) {}

AuthenticationClientImpl::~AuthenticationClientImpl() {
  pending_requests_->CancelAllAndWait();
}

olp::client::HttpResponse AuthenticationClientImpl::CallAuth(
    const client::OlpClient& client, const std::string& endpoint,
    client::CancellationContext context,
    const AuthenticationCredentials& credentials,
    client::OlpClient::RequestBodyType body, std::time_t timestamp,
    const std::string& content_type) {
  // When credentials specify authentication endpoint, it means that
  // Authorization header must be created for the corresponding host.
  const auto url = [&]() {
    if (!credentials.GetEndpointUrl().empty()) {
      return credentials.GetEndpointUrl();
    }
    return settings_.token_endpoint_url + endpoint;
  }();

  auto auth_header =
      GenerateAuthorizationHeader(credentials, url, timestamp, GenerateUid());

  client::OlpClient::ParametersType headers = {
      {http::kAuthorizationHeader, std::move(auth_header)}};

  return client.CallApi(endpoint, "POST", {}, std::move(headers), {},
                        std::move(body), content_type, std::move(context));
}

SignInResult AuthenticationClientImpl::ParseAuthResponse(
    int status, std::stringstream& auth_response) {
  auto document = std::make_shared<rapidjson::Document>();
  rapidjson::IStreamWrapper stream(auth_response);
  document->ParseStream(stream);
  return std::make_shared<SignInResultImpl>(
      status, olp::http::HttpErrorToString(status), document);
}

SignInUserResult AuthenticationClientImpl::ParseUserAuthResponse(
    int status, std::stringstream& auth_response) {
  auto document = std::make_shared<rapidjson::Document>();
  rapidjson::IStreamWrapper stream(auth_response);
  document->ParseStream(stream);
  return std::make_shared<SignInUserResultImpl>(
      status, olp::http::HttpErrorToString(status), document);
}

template <typename SignInResponseType>
Response<SignInResponseType> AuthenticationClientImpl::GetSignInResponse(
    const client::HttpResponse& auth_response,
    const client::CancellationContext& context, const std::string& key) {
  const auto status = auth_response.GetStatus();

  // If a timeout occurred, the cancellation is done through the context.
  // So this case needs to be handled independently of context state.
  if (status != static_cast<int>(http::ErrorCode::TIMEOUT_ERROR) &&
      context.IsCancelled()) {
    return client::ApiError::Cancelled();
  }

  auto result = FindInCache<SignInResponseType>(key);
  if (result) {
    return *result;
  }

  // Auth response message may be empty in case of unknown errors.
  // Fill in the message as a status string representation in this case.
  std::string message;
  auth_response.GetResponse(message);
  if (message.empty()) {
    message = http::HttpErrorToString(status);
  }

  return client::ApiError(status, message);
}

template <>
boost::optional<SignInResult> AuthenticationClientImpl::FindInCache(
    const std::string& key) {
  return client_token_cache_->locked(
      [&](utils::LruCache<std::string, SignInResult>& cache) {
        auto it = cache.Find(key);
        return it != cache.end() ? boost::make_optional(it->value())
                                 : boost::none;
      });
}

template <>
boost::optional<SignInUserResult> AuthenticationClientImpl::FindInCache(
    const std::string& key) {
  return user_token_cache_->locked(
      [&](utils::LruCache<std::string, SignInUserResult>& cache) {
        auto it = cache.Find(key);
        return it != cache.end() ? boost::make_optional(it->value())
                                 : boost::none;
      });
}

template <>
void AuthenticationClientImpl::StoreInCache(const std::string& key,
                                            SignInResult response) {
  // Cache the response
  client_token_cache_->locked(
      [&](utils::LruCache<std::string, SignInResult>& cache) {
        return cache.InsertOrAssign(key, response);
      });
}

template <>
void AuthenticationClientImpl::StoreInCache(const std::string& key,
                                            SignInUserResult response) {
  // Cache the response
  user_token_cache_->locked(
      [&](utils::LruCache<std::string, SignInUserResult>& cache) {
        return cache.InsertOrAssign(key, response);
      });
}

client::CancellationToken AuthenticationClientImpl::SignInClient(
    AuthenticationCredentials credentials, SignInProperties properties,
    SignInClientCallback callback) {
  auto task = [=](client::CancellationContext context) -> SignInClientResponse {
    if (!settings_.network_request_handler) {
      return client::ApiError::NetworkConnection(
          "Cannot sign in while offline");
    }

    if (context.IsCancelled()) {
      return client::ApiError::Cancelled();
    }

    auto olp_client_host = settings_.token_endpoint_url;
    auto endpoint = kOauthEndpoint;
    // If credentials contain URL for the token endpoint then override default
    // endpoint with it. Construction of the `OlpClient` requires the host part
    // of URL, while `CallAuth` method - the rest of URL, hence we need to split
    // URL passed in the Credentials object.
    const auto credentials_endpoint = credentials.GetEndpointUrl();
    const auto maybe_host_and_rest =
        olp::utils::Url::ParseHostAndRest(credentials_endpoint);
    if (maybe_host_and_rest) {
      const auto& host_and_rest = maybe_host_and_rest.value();
      olp_client_host = host_and_rest.first;
      endpoint = host_and_rest.second;
    }

    // To pass correct URL we need to create and modify local copy of shared
    // settings object.
    auto settings = settings_;
    settings.token_endpoint_url = olp_client_host;
    auto client = CreateOlpClient(settings, boost::none, false);

    RequestTimer timer = CreateRequestTimer(client, context);

    const auto request_body = GenerateClientBody(properties);
    const auto content_type = DeduceContentType(properties);

    SignInClientResponse response;

    const auto& retry_settings = settings_.retry_settings;

    for (auto retry = 0; retry < retry_settings.max_attempts; ++retry) {
      if (context.IsCancelled()) {
        return client::ApiError::Cancelled();
      }

      auto auth_response =
          CallAuth(client, endpoint, context, credentials, request_body,
                   timer.GetRequestTime(), content_type);

      const auto status = auth_response.GetStatus();
      if (status < 0) {
        response = GetSignInResponse<SignInResult>(auth_response, context,
                                                   credentials.GetKey());
      } else {
        response = ParseAuthResponse(status, auth_response.GetRawResponse());
      }

      if (retry_settings.retry_condition(auth_response)) {
        RetryDelay(retry_settings, retry);
        continue;
      }

      // In case we can't authorize with system time, retry with the server
      // time from response headers (if available).
      if (HasWrongTimestamp(response.GetResult())) {
        auto server_time = GetTimestampFromHeaders(auth_response.GetHeaders());
        if (server_time) {
          timer = RequestTimer(*server_time);
          continue;
        }
      }

      if (status == http::HttpStatusCode::OK) {
        StoreInCache(credentials.GetKey(), response.GetResult());
      }

      break;
    }

    return response;
  };

  return AddTask(settings_.task_scheduler, pending_requests_, std::move(task),
                 std::move(callback));
}

TimeResponse AuthenticationClientImpl::ParseTimeResponse(
    std::stringstream& payload) {
  rapidjson::Document document;
  rapidjson::IStreamWrapper stream(payload);
  document.ParseStream(stream);

  if (!document.IsObject()) {
    return client::ApiError(client::ErrorCode::InternalFailure,
                            "JSON document root is not an Object type");
  }

  const auto timestamp_it = document.FindMember("timestamp");
  if (timestamp_it == document.MemberEnd() || !timestamp_it->value.IsUint()) {
    return client::ApiError(
        client::ErrorCode::InternalFailure,
        "JSON document must contain timestamp integer field");
  }

  return timestamp_it->value.GetUint();
}

TimeResponse AuthenticationClientImpl::GetTimeFromServer(
    client::CancellationContext context,
    const client::OlpClient& client) const {
  auto http_result = client.CallApi(kTimestampEndpoint, "GET", {}, {}, {},
                                    nullptr, {}, context);

  if (http_result.GetStatus() != http::HttpStatusCode::OK) {
    auto response = http_result.GetResponseAsString();
    OLP_SDK_LOG_WARNING_F(
        kLogTag, "Failed to get time from server, status=%d, response='%s'",
        http_result.GetStatus(), response.c_str());
    return client::ApiError(http_result.GetStatus(), response);
  }

  auto server_time = ParseTimeResponse(http_result.GetRawResponse());

  if (!server_time) {
    const auto& error = server_time.GetError();
    const auto& message = error.GetMessage();
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "Failed to decode time from server, message='%s'",
                          message.c_str());
  }

  return server_time;
}

client::CancellationToken AuthenticationClientImpl::SignInHereUser(
    const AuthenticationCredentials& credentials,
    const UserProperties& properties, const SignInUserCallback& callback) {
  return HandleUserRequest(credentials, kOauthEndpoint,
                           GenerateUserBody(properties), callback);
}

client::CancellationToken AuthenticationClientImpl::SignInFederated(
    AuthenticationCredentials credentials, std::string request_body,
    SignInUserCallback callback) {
  auto payload = std::make_shared<RequestBodyData>(request_body.size());
  std::memcpy(payload->data(), request_body.data(), payload->size());
  return HandleUserRequest(credentials, kOauthEndpoint, payload, callback);
}

client::CancellationToken AuthenticationClientImpl::SignInApple(
    AppleSignInProperties properties, SignInUserCallback callback) {
  auto request_body = GenerateAppleSignInBody(properties);

  auto task = [=](client::CancellationContext context) -> SignInUserResponse {
    if (!settings_.network_request_handler) {
      return client::ApiError::NetworkConnection(
          "Cannot handle user request while offline");
    }

    if (context.IsCancelled()) {
      return client::ApiError::Cancelled();
    }

    auto client = CreateOlpClient(settings_, boost::none);

    auto auth_response = CallApi(client, kOauthEndpoint, context,
                                 properties.GetAccessToken(), request_body);

    auto status = auth_response.GetStatus();
    if (status < 0) {
      return GetSignInResponse<SignInUserResult>(auth_response, context,
                                                 properties.GetClientId());
    }

    auto response =
        ParseUserAuthResponse(status, auth_response.GetRawResponse());

    if (status == http::HttpStatusCode::OK) {
      StoreInCache(properties.GetClientId(), response);
    }

    return response;
  };

  return AddTask(settings_.task_scheduler, pending_requests_, std::move(task),
                 std::move(callback));
}

client::CancellationToken AuthenticationClientImpl::SignInRefresh(
    const AuthenticationCredentials& credentials,
    const RefreshProperties& properties, const SignInUserCallback& callback) {
  return HandleUserRequest(credentials, kOauthEndpoint,
                           GenerateRefreshBody(properties), callback);
}

client::CancellationToken AuthenticationClientImpl::SignInFederated(
    const AuthenticationCredentials& credentials,
    const FederatedSignInType& type, const FederatedProperties& properties,
    const SignInUserCallback& callback) {
  return HandleUserRequest(credentials, kOauthEndpoint,
                           GenerateFederatedBody(type, properties), callback);
}

client::CancellationToken AuthenticationClientImpl::AcceptTerms(
    const AuthenticationCredentials& credentials,
    const std::string& reacceptance_token, const SignInUserCallback& callback) {
  return HandleUserRequest(credentials, kTermsEndpoint,
                           GenerateAcceptTermBody(reacceptance_token),
                           callback);
}

client::CancellationToken AuthenticationClientImpl::HandleUserRequest(
    const AuthenticationCredentials& credentials, const std::string& endpoint,
    const client::OlpClient::RequestBodyType& request_body,
    const SignInUserCallback& callback) {
  auto task = [=](client::CancellationContext context) -> SignInUserResponse {
    if (!settings_.network_request_handler) {
      return client::ApiError::NetworkConnection(
          "Cannot handle user request while offline");
    }

    if (context.IsCancelled()) {
      return client::ApiError::Cancelled();
    }

    auto client = CreateOlpClient(settings_, boost::none, false);

    RequestTimer timer = CreateRequestTimer(client, context);

    SignInUserResult response;

    const auto& retry_settings = settings_.retry_settings;

    for (auto retry = 0; retry < retry_settings.max_attempts; ++retry) {
      if (context.IsCancelled()) {
        return client::ApiError::Cancelled();
      }

      auto auth_response = CallAuth(client, endpoint, context, credentials,
                                    request_body, timer.GetRequestTime());

      auto status = auth_response.GetStatus();
      if (status < 0) {
        return GetSignInResponse<SignInUserResult>(auth_response, context,
                                                   credentials.GetKey());
      }

      response = ParseUserAuthResponse(status, auth_response.GetRawResponse());

      if (retry_settings.retry_condition(auth_response)) {
        RetryDelay(retry_settings, retry);
        continue;
      }

      // In case we can't authorize with system time, retry with the server
      // time from response headers (if available).
      if (HasWrongTimestamp(response)) {
        auto server_time = GetTimestampFromHeaders(auth_response.GetHeaders());
        if (server_time) {
          timer = RequestTimer(*server_time);
          continue;
        }
      }

      if (status == http::HttpStatusCode::OK) {
        StoreInCache(credentials.GetKey(), response);
      }

      break;
    }

    return response;
  };

  return AddTask(settings_.task_scheduler, pending_requests_, std::move(task),
                 callback);
}

client::CancellationToken AuthenticationClientImpl::SignUpHereUser(
    const AuthenticationCredentials& credentials,
    const SignUpProperties& properties, const SignUpCallback& callback) {
  using ResponseType = client::ApiResponse<SignUpResult, client::ApiError>;

  auto signup_task = [=](client::CancellationContext context) -> ResponseType {
    if (!settings_.network_request_handler) {
      return client::ApiError::NetworkConnection(
          "Cannot sign up while offline");
    }

    if (context.IsCancelled()) {
      return client::ApiError::Cancelled();
    }

    auto client = CreateOlpClient(settings_, {}, false);

    const auto url = settings_.token_endpoint_url + kUserEndpoint;
    auto auth_header = GenerateAuthorizationHeader(
        credentials, url, std::time(nullptr), GenerateUid());

    client::OlpClient::ParametersType headers = {
        {http::kAuthorizationHeader, std::move(auth_header)}};

    auto signup_response = client.CallApi(kUserEndpoint, "POST", {}, headers,
                                          {}, GenerateSignUpBody(properties),
                                          kApplicationJson, std::move(context));

    const auto status = signup_response.GetStatus();
    std::string response_text;
    signup_response.GetResponse(response_text);
    if (status < 0) {
      return client::ApiError(status, response_text);
    }

    auto document = std::make_shared<rapidjson::Document>();
    document->Parse(response_text.c_str());
    return {std::make_shared<SignUpResultImpl>(
        status, olp::http::HttpErrorToString(status), document)};
  };

  return AddTask(settings_.task_scheduler, pending_requests_,
                 std::move(signup_task), callback);
}

client::CancellationToken AuthenticationClientImpl::SignOut(
    const AuthenticationCredentials& credentials,
    const std::string& access_token, const SignOutUserCallback& callback) {
  OLP_SDK_CORE_UNUSED(credentials);
  using ResponseType = client::ApiResponse<SignOutResult, client::ApiError>;

  auto sign_out_task =
      [=](client::CancellationContext context) -> ResponseType {
    if (!settings_.network_request_handler) {
      return client::ApiError::NetworkConnection(
          "Cannot sign out while offline");
    }

    if (context.IsCancelled()) {
      return client::ApiError::Cancelled();
    }

    client::AuthenticationSettings auth_settings;
    auth_settings.token_provider =
        [&access_token](client::CancellationContext&) {
          return client::OauthToken(access_token, kMaxTime);
        };

    auto client = CreateOlpClient(settings_, auth_settings, false);

    auto signout_response = client.CallApi(kSignoutEndpoint, "POST", {}, {}, {},
                                           {}, {}, std::move(context));

    const auto status = signout_response.GetStatus();
    std::string response_text;
    signout_response.GetResponse(response_text);
    if (status < 0) {
      return client::ApiError(status, response_text);
    }

    auto document = std::make_shared<rapidjson::Document>();
    document->Parse(response_text.c_str());
    return {std::make_shared<SignOutResultImpl>(
        status, olp::http::HttpErrorToString(status), document)};
  };

  return AddTask(settings_.task_scheduler, pending_requests_,
                 std::move(sign_out_task), callback);
}

client::CancellationToken AuthenticationClientImpl::IntrospectApp(
    std::string access_token, IntrospectAppCallback callback) {
  using ResponseType =
      client::ApiResponse<IntrospectAppResult, client::ApiError>;

  auto introspect_app_task =
      [=](client::CancellationContext context) -> ResponseType {
    if (!settings_.network_request_handler) {
      return client::ApiError::NetworkConnection(
          "Cannot introspect app while offline");
    }

    client::AuthenticationSettings auth_settings;
    auth_settings.token_provider =
        [&access_token](client::CancellationContext&) {
          return client::OauthToken(access_token, kMaxTime);
        };

    auto client = CreateOlpClient(settings_, auth_settings);

    auto http_result = client.CallApi(kIntrospectAppEndpoint, "GET", {}, {}, {},
                                      nullptr, {}, context);

    rapidjson::Document document;
    rapidjson::IStreamWrapper stream(http_result.GetRawResponse());
    document.ParseStream(stream);
    if (http_result.GetStatus() != http::HttpStatusCode::OK) {
      // HttpResult response can be error message or valid json with it.
      std::string msg = http_result.GetResponseAsString();
      if (!document.HasParseError() && document.HasMember(Constants::MESSAGE)) {
        msg = document[Constants::MESSAGE].GetString();
      }
      return client::ApiError({http_result.GetStatus(), msg});
    }

    if (document.HasParseError()) {
      return client::ApiError({static_cast<int>(http::ErrorCode::UNKNOWN_ERROR),
                               "Failed to parse response"});
    }

    return GetIntrospectAppResult(document);
  };

  return AddTask(settings_.task_scheduler, pending_requests_,
                 std::move(introspect_app_task), std::move(callback));
}

client::CancellationToken AuthenticationClientImpl::Authorize(
    std::string access_token, AuthorizeRequest request,
    AuthorizeCallback callback) {
  using ResponseType = client::ApiResponse<AuthorizeResult, client::ApiError>;

  auto task = [=](client::CancellationContext context) -> ResponseType {
    if (!settings_.network_request_handler) {
      return client::ApiError::NetworkConnection(
          "Can not send request while offline");
    }

    client::AuthenticationSettings auth_settings;
    auth_settings.token_provider =
        [&access_token](client::CancellationContext&) {
          return client::OauthToken(access_token, kMaxTime);
        };

    auto client = CreateOlpClient(settings_, auth_settings);

    auto http_result = client.CallApi(kDecisionEndpoint, "POST", {}, {}, {},
                                      GenerateAuthorizeBody(request),
                                      kApplicationJson, context);

    rapidjson::Document document;
    rapidjson::IStreamWrapper stream(http_result.GetRawResponse());
    document.ParseStream(stream);
    if (http_result.GetStatus() != http::HttpStatusCode::OK) {
      // HttpResult response can be error message or valid json with it.
      std::string msg = http_result.GetResponseAsString();
      if (!document.HasParseError() && document.HasMember(Constants::MESSAGE)) {
        msg = document[Constants::MESSAGE].GetString();
      }
      return client::ApiError({http_result.GetStatus(), msg});
    } else if (!document.HasParseError() &&
               document.HasMember(Constants::ERROR_CODE) &&
               document[Constants::ERROR_CODE].IsInt()) {
      std::string msg =
          "Error code: " +
          std::to_string(document[Constants::ERROR_CODE].GetInt());
      if (document.HasMember(Constants::MESSAGE)) {
        msg.append(" (");
        msg.append(document[Constants::MESSAGE].GetString());
        msg.append(")");
      }

      return client::ApiError(
          {static_cast<int>(http::ErrorCode::UNKNOWN_ERROR), msg});
    }

    if (document.HasParseError()) {
      return client::ApiError({static_cast<int>(http::ErrorCode::UNKNOWN_ERROR),
                               "Failed to parse response"});
    }

    return GetAuthorizeResult(document);
  };

  return AddTask(settings_.task_scheduler, pending_requests_, std::move(task),
                 std::move(callback));
}

client::CancellationToken AuthenticationClientImpl::GetMyAccount(
    std::string access_token, UserAccountInfoCallback callback) {
  auto task =
      [=](client::CancellationContext context) -> UserAccountInfoResponse {
    if (!settings_.network_request_handler) {
      return client::ApiError::NetworkConnection(
          "Can not send request while offline");
    }

    client::AuthenticationSettings auth_settings;
    auth_settings.token_provider =
        [&access_token](client::CancellationContext&) {
          return client::OauthToken(access_token, kMaxTime);
        };

    auto client = CreateOlpClient(settings_, auth_settings);

    auto http_result =
        client.CallApi(kMyAccountEndpoint, "GET", {}, {}, {}, {}, {}, context);

    return GetUserAccountInfoResponse(http_result);
  };

  return AddTask(settings_.task_scheduler, pending_requests_, std::move(task),
                 std::move(callback));
}

client::OlpClient::RequestBodyType AuthenticationClientImpl::GenerateClientBody(
    const SignInProperties& properties) {
  if (properties.custom_body) {
    const auto& content = properties.custom_body.value();
    return std::make_shared<RequestBodyData>(content.data(),
                                             content.data() + content.size());
  };
  rapidjson::StringBuffer data;
  rapidjson::Writer<rapidjson::StringBuffer> writer(data);
  writer.StartObject();

  writer.Key(kGrantType);
  writer.String(kClientGrantType);

  auto expires_in = static_cast<unsigned int>(properties.expires_in.count());
  if (expires_in > 0) {
    writer.Key(Constants::EXPIRES_IN);
    writer.Uint(expires_in);
  }

  if (properties.scope) {
    writer.Key(kScope);
    writer.String(properties.scope.get().c_str());
  }

  if (properties.device_id) {
    writer.Key(kDeviceId);
    writer.String(properties.device_id.get().c_str());
  }

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<RequestBodyData>(content, content + data.GetSize());
}

client::OlpClient::RequestBodyType AuthenticationClientImpl::GenerateUserBody(
    const UserProperties& properties) {
  rapidjson::StringBuffer data;
  rapidjson::Writer<rapidjson::StringBuffer> writer(data);
  writer.StartObject();

  writer.Key(kGrantType);
  writer.String(kUserGrantType);

  if (!properties.email.empty()) {
    writer.Key(kEmail);
    writer.String(properties.email.c_str());
  }
  if (!properties.password.empty()) {
    writer.Key(kPassword);
    writer.String(properties.password.c_str());
  }
  if (properties.expires_in > 0) {
    writer.Key(Constants::EXPIRES_IN);
    writer.Uint(properties.expires_in);
  }
  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<RequestBodyData>(content, content + data.GetSize());
}

client::OlpClient::RequestBodyType
AuthenticationClientImpl::GenerateFederatedBody(
    const FederatedSignInType type, const FederatedProperties& properties) {
  rapidjson::StringBuffer data;
  rapidjson::Writer<rapidjson::StringBuffer> writer(data);
  writer.StartObject();

  writer.Key(kGrantType);
  switch (type) {
    case FederatedSignInType::FacebookSignIn:
      writer.String(kFacebookGrantType);
      break;
    case FederatedSignInType::ArcgisSignIn:
      writer.String(kArcgisGrantType);
      break;
    default:
      return nullptr;
  }

  if (!properties.access_token.empty()) {
    writer.Key(Constants::ACCESS_TOKEN);
    writer.String(properties.access_token.c_str());
  }
  if (!properties.country_code.empty()) {
    writer.Key(kCountryCode);
    writer.String(properties.country_code.c_str());
  }
  if (!properties.language.empty()) {
    writer.Key(kLanguage);
    writer.String(properties.language.c_str());
  }
  if (!properties.email.empty()) {
    writer.Key(kEmail);
    writer.String(properties.email.c_str());
  }
  if (properties.expires_in > 0) {
    writer.Key(Constants::EXPIRES_IN);
    writer.Uint(properties.expires_in);
  }

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<RequestBodyData>(content, content + data.GetSize());
}

client::OlpClient::RequestBodyType
AuthenticationClientImpl::GenerateRefreshBody(
    const RefreshProperties& properties) {
  rapidjson::StringBuffer data;
  rapidjson::Writer<rapidjson::StringBuffer> writer(data);
  writer.StartObject();

  writer.Key(kGrantType);
  writer.String(kRefreshGrantType);

  if (!properties.access_token.empty()) {
    writer.Key(Constants::ACCESS_TOKEN);
    writer.String(properties.access_token.c_str());
  }
  if (!properties.refresh_token.empty()) {
    writer.Key(Constants::REFRESH_TOKEN);
    writer.String(properties.refresh_token.c_str());
  }
  if (properties.expires_in > 0) {
    writer.Key(Constants::EXPIRES_IN);
    writer.Uint(properties.expires_in);
  }
  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<RequestBodyData>(content, content + data.GetSize());
}

client::OlpClient::RequestBodyType AuthenticationClientImpl::GenerateSignUpBody(
    const SignUpProperties& properties) {
  rapidjson::StringBuffer data;
  rapidjson::Writer<rapidjson::StringBuffer> writer(data);
  writer.StartObject();

  if (!properties.email.empty()) {
    writer.Key(kEmail);
    writer.String(properties.email.c_str());
  }
  if (!properties.password.empty()) {
    writer.Key(kPassword);
    writer.String(properties.password.c_str());
  }
  if (!properties.date_of_birth.empty()) {
    writer.Key(kDateOfBirth);
    writer.String(properties.date_of_birth.c_str());
  }
  if (!properties.first_name.empty()) {
    writer.Key(kFirstName);
    writer.String(properties.first_name.c_str());
  }
  if (!properties.last_name.empty()) {
    writer.Key(kLastName);
    writer.String(properties.last_name.c_str());
  }
  if (!properties.country_code.empty()) {
    writer.Key(kCountryCode);
    writer.String(properties.country_code.c_str());
  }
  if (!properties.language.empty()) {
    writer.Key(kLanguage);
    writer.String(properties.language.c_str());
  }
  if (properties.marketing_enabled) {
    writer.Key(kMarketingEnabled);
    writer.Bool(true);
  }
  if (!properties.phone_number.empty()) {
    writer.Key(kPhoneNumber);
    writer.String(properties.phone_number.c_str());
  }
  if (!properties.realm.empty()) {
    writer.Key(kRealm);
    writer.String(properties.realm.c_str());
  }
  if (!properties.invite_token.empty()) {
    writer.Key(kInviteToken);
    writer.String(properties.invite_token.c_str());
  }

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<RequestBodyData>(content, content + data.GetSize());
}

client::OlpClient::RequestBodyType
AuthenticationClientImpl::GenerateAcceptTermBody(
    const std::string& reacceptance_token) {
  rapidjson::StringBuffer data;
  rapidjson::Writer<rapidjson::StringBuffer> writer(data);
  writer.StartObject();

  writer.Key(kTermsReacceptanceToken);
  writer.String(reacceptance_token.c_str());

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<RequestBodyData>(content, content + data.GetSize());
}

client::OlpClient::RequestBodyType
AuthenticationClientImpl::GenerateAuthorizeBody(
    const AuthorizeRequest& properties) {
  rapidjson::StringBuffer data;
  rapidjson::Writer<rapidjson::StringBuffer> writer(data);
  writer.StartObject();
  writer.Key(kServiceId);
  writer.String(properties.GetServiceId().c_str());
  writer.Key(kActions);
  writer.StartArray();
  for (const auto& action : properties.GetActions()) {
    writer.StartObject();
    writer.Key(kAction);
    writer.String(action.first.c_str());
    if (!action.second.empty()) {
      writer.Key(kResource);
      writer.String(action.second.c_str());
    }
    writer.EndObject();
  }

  writer.EndArray();
  writer.Key(kDiagnostics);
  writer.Bool(properties.GetDiagnostics());
  // default value is 'and', ignore parameter if operator type is 'and'
  if (properties.GetOperatorType() ==
      AuthorizeRequest::DecisionOperatorType::kOr) {
    writer.Key(kOperator);
    writer.String("or");
  }

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<RequestBodyData>(content, content + data.GetSize());
}

std::string AuthenticationClientImpl::GenerateUid() const {
  std::lock_guard<std::mutex> lock(token_mutex_);
  {
    static boost::uuids::random_generator gen;

    return boost::uuids::to_string(gen());
  }
}

AuthenticationClientImpl::RequestTimer
AuthenticationClientImpl::CreateRequestTimer(
    const client::OlpClient& client,
    client::CancellationContext context) const {
  if (settings_.use_system_time) {
    return RequestTimer();
  }

  auto server_time = GetTimeFromServer(context, client);
  if (!server_time) {
    return RequestTimer();
  }

  return RequestTimer(server_time.GetResult());
}

}  // namespace authentication
}  // namespace olp
