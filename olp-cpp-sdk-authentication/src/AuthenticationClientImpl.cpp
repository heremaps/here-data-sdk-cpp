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

#include "AuthenticationClientImpl.h"

#include <chrono>
#include <iomanip>
#include <sstream>

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

namespace {
namespace client = olp::client;

using olp::authentication::Constants;

// Tags
constexpr auto kApplicationJson = "application/json";
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
constexpr auto kInviteToken = "inviteToken";
constexpr auto kLanguage = "language";
constexpr auto kLastName = "lastname";
constexpr auto kMarketingEnabled = "marketingEnabled";
constexpr auto kPassword = "password";
constexpr auto kPhoneNumber = "phoneNumber";
constexpr auto kRealm = "realm";
constexpr auto kTermsReacceptanceToken = "termsReacceptanceToken";

constexpr auto kClientGrantType = "client_credentials";
constexpr auto kUserGrantType = "password";
constexpr auto kFacebookGrantType = "facebook";
constexpr auto kGoogleGrantType = "google";
constexpr auto kArcgisGrantType = "arcgis";
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

}  // namespace

namespace olp {
namespace authentication {

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
    const client::OlpClient& client, client::CancellationContext context,
    const AuthenticationCredentials& credentials,
    const SignInProperties& properties, std::time_t timestamp) {
  const auto url = settings_.token_endpoint_url + kOauthEndpoint;

  auto auth_header =
      GenerateAuthorizationHeader(credentials, url, timestamp, GenerateUid());

  client::OlpClient::ParametersType headers = {
      {http::kAuthorizationHeader, std::move(auth_header)}};
  if (context.IsCancelled()) {
    return {static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            "Cancelled"};
  }
  auto auth_response =
      client.CallApi(kOauthEndpoint, "POST", {}, std::move(headers), {},
                     GenerateClientBody(properties), kApplicationJson, context);
  return auth_response;
}

SignInResult AuthenticationClientImpl::ParseAuthResponse(
    int status, std::stringstream& auth_response) {
  auto document = std::make_shared<rapidjson::Document>();
  rapidjson::IStreamWrapper stream(auth_response);
  document->ParseStream(stream);
  auto resp_impl = std::make_shared<SignInResultImpl>(
      status, olp::http::HttpErrorToString(status), document);
  SignInResult response(resp_impl);

  return response;
}

SignInClientResponse AuthenticationClientImpl::FindSingInResponseInCache(
    int status, const std::string& error_message,
    const AuthenticationCredentials& credentials) {
  if (status == static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR)) {
    return {{status, error_message}};
  }

  auto cached_response = client_token_cache_->locked(
      [&](utils::LruCache<std::string, SignInResult>& c)
          -> boost::optional<SignInResult> {
        auto it = c.Find(credentials.GetKey());
        return it != c.end() ? boost::make_optional(it->value()) : boost::none;
      });

  if (!cached_response) {
    return {{status, error_message}};
  }

  return *cached_response;
}

client::CancellationToken AuthenticationClientImpl::SignInClient(
    AuthenticationCredentials credentials, SignInProperties properties,
    SignInClientCallback callback) {
  auto sign_in_task =
      [=](client::CancellationContext context) -> SignInClientResponse {
    {
      if (!settings_.network_request_handler) {
        return {{static_cast<int>(http::ErrorCode::IO_ERROR),
                 "Cannot sign in while offline"}};
      }

      if (context.IsCancelled()) {
        return {
            {static_cast<int>(http::ErrorCode::CANCELLED_ERROR), "Cancelled"}};
      }

      client::OlpClient client = CreateOlpClient(settings_, boost::none);
      std::time_t timestamp = std::chrono::system_clock::to_time_t(
          std::chrono::system_clock::now());

      if (!settings_.use_system_time) {
        auto server_time = GetTimeFromServer(context, client);
        if (server_time.IsSuccessful()) {
          timestamp = server_time.GetResult();
        } else {
          OLP_SDK_LOG_WARNING(
              kLogTag,
              "Failed to get time from server, system time will be used");
        }
      }

      auto auth_response =
          CallAuth(client, context, credentials, properties, timestamp);

      auto status = auth_response.status;

      if (status < 0) {
        return FindSingInResponseInCache(status, auth_response.response.str(),
                                         credentials);
      }

      if (context.IsCancelled()) {
        return {{static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
                 "Cancelled"}};
      }

      auto response = ParseAuthResponse(status, auth_response.response);

      if (settings_.use_system_time &&
          status == http::HttpStatusCode::UNAUTHORIZED &&
          response.GetErrorResponse().code == kErrorWrongTimestamp) {
        timestamp = GetTimestampFromHeaders(auth_response.headers);
        if (timestamp != 0) {
          auth_response =
              CallAuth(client, context, credentials, properties, timestamp);
          status = auth_response.status;
          if (status < 0) {
            return FindSingInResponseInCache(
                status, auth_response.response.str(), credentials);
          }

          response = ParseAuthResponse(status, auth_response.response);
        }
      }

      if (status == http::HttpStatusCode::OK) {
        // Cache the response
        client_token_cache_->locked(
            [&](utils::LruCache<std::string, SignInResult>& c) {
              return c.InsertOrAssign(credentials.GetKey(), response);
            });
      }
      return response;
    }
  };

  return AddTask(settings_.task_scheduler, pending_requests_,
                 std::move(sign_in_task), std::move(callback));
}

TimeResponse AuthenticationClientImpl::ParseTimeResponse(
    std::stringstream& payload) {
  rapidjson::Document document;
  rapidjson::IStreamWrapper stream(payload);
  document.ParseStream(stream);

  if (!document.IsObject()) {
    return AuthenticationError(client::ErrorCode::InternalFailure,
                               "JSON document root is not an Object type");
  }

  const auto timestamp_it = document.FindMember("timestamp");
  if (timestamp_it == document.MemberEnd() || !timestamp_it->value.IsUint()) {
    return AuthenticationError(
        client::ErrorCode::InternalFailure,
        "JSON document must contain timestamp integer field");
  }

  return timestamp_it->value.GetUint();
}

TimeResponse AuthenticationClientImpl::GetTimeFromServer(
    client::CancellationContext context, const client::OlpClient& client) {
  auto http_result = client.CallApi(kTimestampEndpoint, "GET", {}, {}, {},
                                    nullptr, {}, context);

  if (http_result.status != http::HttpStatusCode::OK) {
    return AuthenticationError(http_result.status, http_result.response.str());
  }

  return ParseTimeResponse(http_result.response);
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
  auto payload =
      std::make_shared<std::vector<unsigned char>>(request_body.size());
  std::memcpy(payload->data(), request_body.data(), payload->size());
  return HandleUserRequest(credentials, kOauthEndpoint, payload, callback);
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
    const http::NetworkRequest::RequestBodyType& request_body,
    const SignInUserCallback& callback) {
  if (!settings_.network_request_handler) {
    ExecuteOrSchedule(settings_.task_scheduler, [callback] {
      AuthenticationError result({static_cast<int>(http::ErrorCode::IO_ERROR),
                                  "Cannot handle user request while offline"});
      callback(result);
    });
    return client::CancellationToken();
  }
  std::weak_ptr<http::Network> weak_network(settings_.network_request_handler);
  std::string url = settings_.token_endpoint_url;
  url.append(endpoint);
  http::NetworkRequest request(url);
  http::NetworkSettings network_settings;
  if (settings_.network_proxy_settings) {
    network_settings.WithProxySettings(settings_.network_proxy_settings.get());
  }
  request.WithVerb(http::NetworkRequest::HttpVerb::POST);

  auto auth_header = GenerateAuthorizationHeader(
      credentials, url, std::time(nullptr), GenerateUid());

  request.WithHeader(http::kAuthorizationHeader, std::move(auth_header));
  request.WithHeader(http::kContentTypeHeader, kApplicationJson);
  request.WithHeader(http::kUserAgentHeader, http::kOlpSdkUserAgent);
  request.WithSettings(std::move(network_settings));

  std::shared_ptr<std::stringstream> payload =
      std::make_shared<std::stringstream>();
  request.WithBody(request_body);
  auto cache = user_token_cache_;
  auto send_outcome = settings_.network_request_handler->Send(
      request, payload,
      [callback, payload, credentials,
       cache](const http::NetworkResponse& network_response) {
        auto response_status = network_response.GetStatus();
        auto error_msg = network_response.GetError();

        // Network not available, use cached token if available
        if (response_status < 0) {
          // Request cancelled, return
          if (response_status ==
              static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR)) {
            callback({{response_status, error_msg}});
            return;
          }

          auto cached_response_found = cache->locked(
              [credentials,
               callback](utils::LruCache<std::string, SignInUserResult>& c) {
                auto it = c.Find(credentials.GetKey());
                if (it != c.end()) {
                  callback(it->value());
                  return true;
                }
                return false;
              });

          if (!cached_response_found) {
            // Return an error response
            SignInUserResult response(std::make_shared<SignInUserResultImpl>(
                response_status, error_msg));
            callback(response);
          }

          return;
        }

        auto document = std::make_shared<rapidjson::Document>();
        rapidjson::IStreamWrapper stream(*payload);
        document->ParseStream(stream);

        std::shared_ptr<SignInUserResultImpl> resp_impl =
            std::make_shared<SignInUserResultImpl>(response_status, error_msg,
                                                   document);
        SignInUserResult response(resp_impl);

        if (!resp_impl->IsValid()) {
          callback(response);
          return;
        }

        if (response_status == http::HttpStatusCode::OK) {
          cache->locked([credentials, &response](
                            utils::LruCache<std::string, SignInUserResult>& c) {
            return c.InsertOrAssign(credentials.GetKey(), response);
          });
        }
        callback(response);
      });

  if (!send_outcome.IsSuccessful()) {
    ExecuteOrSchedule(settings_.task_scheduler, [send_outcome, callback] {
      std::string error_message =
          ErrorCodeToString(send_outcome.GetErrorCode());
      AuthenticationError result({static_cast<int>(send_outcome.GetErrorCode()),
                                  std::move(error_message)});
      callback(result);
    });
    return client::CancellationToken();
  }

  auto request_id = send_outcome.GetRequestId();
  return client::CancellationToken([weak_network, request_id]() {
    auto network = weak_network.lock();

    if (network) {
      network->Cancel(request_id);
    }
  });
}

client::CancellationToken AuthenticationClientImpl::SignUpHereUser(
    const AuthenticationCredentials& credentials,
    const SignUpProperties& properties, const SignUpCallback& callback) {
  if (!settings_.network_request_handler) {
    ExecuteOrSchedule(settings_.task_scheduler, [callback] {
      AuthenticationError result({static_cast<int>(http::ErrorCode::IO_ERROR),
                                  "Cannot sign up while offline"});
      callback(result);
    });
    return client::CancellationToken();
  }
  std::weak_ptr<http::Network> weak_network(settings_.network_request_handler);
  std::string url = settings_.token_endpoint_url;
  url.append(kUserEndpoint);
  http::NetworkRequest request(url);
  http::NetworkSettings network_settings;
  if (settings_.network_proxy_settings) {
    network_settings.WithProxySettings(settings_.network_proxy_settings.get());
  }
  request.WithVerb(http::NetworkRequest::HttpVerb::POST);

  auto auth_header = GenerateAuthorizationHeader(
      credentials, url, std::time(nullptr), GenerateUid());

  request.WithHeader(http::kAuthorizationHeader, std::move(auth_header));
  request.WithHeader(http::kContentTypeHeader, kApplicationJson);
  request.WithHeader(http::kUserAgentHeader, http::kOlpSdkUserAgent);
  request.WithSettings(std::move(network_settings));

  std::shared_ptr<std::stringstream> payload =
      std::make_shared<std::stringstream>();
  request.WithBody(GenerateSignUpBody(properties));
  auto send_outcome = settings_.network_request_handler->Send(
      request, payload,
      [callback, payload,
       credentials](const http::NetworkResponse& network_response) {
        auto response_status = network_response.GetStatus();
        auto error_msg = network_response.GetError();

        if (response_status < 0) {
          // Network error response
          AuthenticationError error(response_status, error_msg);
          callback(error);
          return;
        }

        auto document = std::make_shared<rapidjson::Document>();
        rapidjson::IStreamWrapper stream(*payload.get());
        document->ParseStream(stream);

        std::shared_ptr<SignUpResultImpl> resp_impl =
            std::make_shared<SignUpResultImpl>(response_status, error_msg,
                                               document);
        SignUpResult response(resp_impl);
        callback(response);
      });

  if (!send_outcome.IsSuccessful()) {
    ExecuteOrSchedule(settings_.task_scheduler, [send_outcome, callback] {
      std::string error_message =
          ErrorCodeToString(send_outcome.GetErrorCode());
      AuthenticationError result({static_cast<int>(send_outcome.GetErrorCode()),
                                  std::move(error_message)});
      callback(result);
    });
    return client::CancellationToken();
  }

  auto request_id = send_outcome.GetRequestId();
  return client::CancellationToken([weak_network, request_id]() {
    auto network = weak_network.lock();

    if (network) {
      network->Cancel(request_id);
    }
  });
}

client::CancellationToken AuthenticationClientImpl::SignOut(
    const AuthenticationCredentials& credentials,
    const std::string& access_token, const SignOutUserCallback& callback) {
  if (!settings_.network_request_handler) {
    ExecuteOrSchedule(settings_.task_scheduler, [callback] {
      AuthenticationError result({static_cast<int>(http::ErrorCode::IO_ERROR),
                                  "Cannot sign out while offline"});
      callback(result);
    });
    return client::CancellationToken();
  }
  std::weak_ptr<http::Network> weak_network(settings_.network_request_handler);
  std::string url = settings_.token_endpoint_url;
  url.append(kSignoutEndpoint);
  http::NetworkRequest request(url);
  http::NetworkSettings network_settings;
  if (settings_.network_proxy_settings) {
    network_settings.WithProxySettings(settings_.network_proxy_settings.get());
  }
  request.WithVerb(http::NetworkRequest::HttpVerb::POST);
  request.WithHeader(http::kAuthorizationHeader,
                     GenerateBearerHeader(access_token));
  request.WithHeader(http::kUserAgentHeader, http::kOlpSdkUserAgent);
  request.WithSettings(std::move(network_settings));

  std::shared_ptr<std::stringstream> payload =
      std::make_shared<std::stringstream>();
  auto send_outcome = settings_.network_request_handler->Send(
      request, payload,
      [callback, payload,
       credentials](const http::NetworkResponse& network_response) {
        auto response_status = network_response.GetStatus();
        auto error_msg = network_response.GetError();

        if (response_status < 0) {
          // Network error response not available
          AuthenticationError error(response_status, error_msg);
          callback(error);
          return;
        }

        auto document = std::make_shared<rapidjson::Document>();
        rapidjson::IStreamWrapper stream(*payload);
        document->ParseStream(stream);

        std::shared_ptr<SignOutResultImpl> resp_impl =
            std::make_shared<SignOutResultImpl>(response_status, error_msg,
                                                document);
        SignOutResult response(resp_impl);
        callback(response);
      });

  if (!send_outcome.IsSuccessful()) {
    ExecuteOrSchedule(settings_.task_scheduler, [send_outcome, callback] {
      std::string error_message =
          ErrorCodeToString(send_outcome.GetErrorCode());
      AuthenticationError result({static_cast<int>(send_outcome.GetErrorCode()),
                                  std::move(error_message)});
      callback(result);
    });
    return client::CancellationToken();
  }

  auto request_id = send_outcome.GetRequestId();
  return client::CancellationToken([weak_network, request_id]() {
    auto network = weak_network.lock();

    if (network) {
      network->Cancel(request_id);
    }
  });
}

client::CancellationToken AuthenticationClientImpl::IntrospectApp(
    std::string access_token, IntrospectAppCallback callback) {
  using ResponseType =
      client::ApiResponse<IntrospectAppResult, client::ApiError>;

  auto introspect_app_task =
      [=](client::CancellationContext context) -> ResponseType {
    if (!settings_.network_request_handler) {
      return client::ApiError({static_cast<int>(http::ErrorCode::IO_ERROR),
                               "Cannot introspect app while offline"});
    }

    client::AuthenticationSettings auth_settings;
    auth_settings.provider = [&access_token]() { return access_token; };

    client::OlpClient client = CreateOlpClient(settings_, auth_settings);

    auto http_result = client.CallApi(kIntrospectAppEndpoint, "GET", {}, {}, {},
                                      nullptr, {}, context);

    rapidjson::Document document;
    rapidjson::IStreamWrapper stream(http_result.response);
    document.ParseStream(stream);
    if (http_result.status != http::HttpStatusCode::OK) {
      // HttpResult response can be error message or valid json with it.
      std::string msg = http_result.response.str();
      if (!document.HasParseError() && document.HasMember(Constants::MESSAGE)) {
        msg = document[Constants::MESSAGE].GetString();
      }
      return client::ApiError({http_result.status, msg});
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
      return client::ApiError({static_cast<int>(http::ErrorCode::IO_ERROR),
                               "Can not send request while offline"});
    }

    client::AuthenticationSettings auth_settings;
    auth_settings.provider = [&access_token]() { return access_token; };

    client::OlpClient client = CreateOlpClient(settings_, auth_settings);

    auto http_result = client.CallApi(kDecisionEndpoint, "POST", {}, {}, {},
                                      GenerateAuthorizeBody(request),
                                      kApplicationJson, context);

    rapidjson::Document document;
    rapidjson::IStreamWrapper stream(http_result.response);
    document.ParseStream(stream);
    if (http_result.status != http::HttpStatusCode::OK) {
      // HttpResult response can be error message or valid json with it.
      std::string msg = http_result.response.str();
      if (!document.HasParseError() && document.HasMember(Constants::MESSAGE)) {
        msg = document[Constants::MESSAGE].GetString();
      }
      return client::ApiError({http_result.status, msg});
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
      return client::ApiError({static_cast<int>(http::ErrorCode::IO_ERROR),
                               "Can not send request while offline"});
    }

    client::AuthenticationSettings auth_settings;
    auth_settings.provider = [&access_token]() { return access_token; };

    client::OlpClient client = CreateOlpClient(settings_, auth_settings);

    auto http_result =
        client.CallApi(kMyAccountEndpoint, "GET", {}, {}, {}, {}, {}, context);

    return GetUserAccountInfoResponse(http_result);
  };

  return AddTask(settings_.task_scheduler, pending_requests_, std::move(task),
                 std::move(callback));
}

std::string AuthenticationClientImpl::GenerateBearerHeader(
    const std::string& bearer_token) {
  std::string authorization = http::kBearer + std::string(" ");
  authorization += bearer_token;
  return authorization;
}

client::OlpClient::RequestBodyType AuthenticationClientImpl::GenerateClientBody(
    const SignInProperties& properties) {
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
  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<std::vector<unsigned char>>(content,
                                                      content + data.GetSize());
}

http::NetworkRequest::RequestBodyType
AuthenticationClientImpl::GenerateUserBody(const UserProperties& properties) {
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
  return std::make_shared<std::vector<unsigned char>>(content,
                                                      content + data.GetSize());
}

http::NetworkRequest::RequestBodyType
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
    case FederatedSignInType::GoogleSignIn:
      writer.String(kGoogleGrantType);
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
  return std::make_shared<std::vector<unsigned char>>(content,
                                                      content + data.GetSize());
}

http::NetworkRequest::RequestBodyType
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
  return std::make_shared<std::vector<unsigned char>>(content,
                                                      content + data.GetSize());
}

http::NetworkRequest::RequestBodyType
AuthenticationClientImpl::GenerateSignUpBody(
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
  return std::make_shared<std::vector<unsigned char>>(content,
                                                      content + data.GetSize());
}

http::NetworkRequest::RequestBodyType
AuthenticationClientImpl::GenerateAcceptTermBody(
    const std::string& reacceptance_token) {
  rapidjson::StringBuffer data;
  rapidjson::Writer<rapidjson::StringBuffer> writer(data);
  writer.StartObject();

  writer.Key(kTermsReacceptanceToken);
  writer.String(reacceptance_token.c_str());

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<std::vector<unsigned char>>(content,
                                                      content + data.GetSize());
}

client::OlpClient::RequestBodyType
AuthenticationClientImpl::GenerateAuthorizeBody(AuthorizeRequest properties) {
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
  return std::make_shared<std::vector<unsigned char>>(content,
                                                      content + data.GetSize());
}

std::string AuthenticationClientImpl::GenerateUid() const {
  std::lock_guard<std::mutex> lock(token_mutex_);
  {
    static boost::uuids::random_generator gen;

    return boost::uuids::to_string(gen());
  }
}

}  // namespace authentication
}  // namespace olp
