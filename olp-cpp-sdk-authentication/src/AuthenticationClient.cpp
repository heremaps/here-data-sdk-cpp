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

#include "olp/authentication/AuthenticationClient.h"

#include <chrono>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "Constants.h"
#include "Crypto.h"
#include "SignInResultImpl.h"
#include "SignInUserResultImpl.h"
#include "SignOutResultImpl.h"
#include "SignUpResultImpl.h"
#include "olp/authentication/AuthenticationError.h"
#include "olp/core/client/ApiError.h"
#include "olp/core/client/CancellationToken.h"
#include "olp/core/client/ErrorCode.h"
#include "olp/core/http/HttpStatusCode.h"
#include "olp/core/http/Network.h"
#include "olp/core/http/NetworkConstants.h"
#include "olp/core/http/NetworkRequest.h"
#include "olp/core/http/NetworkResponse.h"
#include "olp/core/porting/make_unique.h"
#include "olp/core/thread/Atomic.h"
#include "olp/core/thread/TaskScheduler.h"
#include "olp/core/utils/Base64.h"
#include "olp/core/utils/LruCache.h"
#include "olp/core/utils/Url.h"

using namespace olp::thread;
using namespace olp::utils;

namespace {
// Helper characters
constexpr auto kParamAdd = "&";
constexpr auto kParamComma = ",";
constexpr auto kParamEquals = "=";
constexpr auto kParamQuote = "\"";
constexpr auto kLineFeed = '\n';

// Tags
const std::string kApplicationJson = "application/json";
const std::string kOauthPost = "POST";
const std::string kOauthConsumerKey = "oauth_consumer_key";
const std::string kOauthNonce = "oauth_nonce";
const std::string kOauthSignature = "oauth_signature";
const std::string kOauthTimestamp = "oauth_timestamp";
const std::string kOauthVersion = "oauth_version";
const std::string kOauthSignatureMethod = "oauth_signature_method";
const std::string kOauthEndpoint = "/oauth2/token";
const std::string kSignoutEndpoint = "/logout";
const std::string kTermsEndpoint = "/terms";
const std::string kUserEndpoint = "/user";
const std::string kTimestampEndpoint = "/timestamp";

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

// Values
constexpr auto kVersion = "1.0";
constexpr auto kHmac = "HMAC-SHA256";

void ExecuteOrSchedule(
    std::shared_ptr<olp::thread::TaskScheduler>& task_scheduler,
    olp::thread::TaskScheduler::CallFuncType&& func) {
  if (!task_scheduler) {
    // User didn't specify a TaskScheduler, execute sync
    func();
    return;
  }

  // Schedule for async execution
  task_scheduler->ScheduleTask(std::move(func));
}

}  // namespace

namespace olp {
namespace authentication {

enum class FederatedSignInType { FacebookSignIn, GoogleSignIn, ArcgisSignIn };

class AuthenticationClient::Impl final {
 public:
  /// The sign in cache alias type
  using SignInCacheType = Atomic<utils::LruCache<std::string, SignInResult>>;

  /// The sign in user cache alias type
  using SignInUserCacheType =
      Atomic<utils::LruCache<std::string, SignInUserResult>>;

  /**
   * @brief Constructor
   * @param token_cache_limit Maximum number of tokens that will be cached.
   */
  Impl(const std::string& authentication_server_url, size_t token_cache_limit);

  /**
   * @brief Constructor
   * @param settings The authentication settings that can be used to configure
   * the `Impl`  instance.
   */
  Impl(AuthenticationSettings settings);

  /**
   * @brief Sign in with client credentials
   * @param credentials Client credentials obtained when registering application
   *                    on HERE developer portal.
   * @param callback  The method to be called when request is completed.
   * @param expiresIn The number of seconds until the new access token expires.
   * @return CancellationToken that can be used to cancel the request.
   */
  client::CancellationToken SignInClient(
      AuthenticationCredentials credentials, SignInProperties properties,
      AuthenticationClient::SignInClientCallback callback);

  client::CancellationToken SignInHereUser(
      const AuthenticationCredentials& credentials,
      const UserProperties& properties,
      const AuthenticationClient::SignInUserCallback& callback);

  client::CancellationToken SignInFederated(
      AuthenticationCredentials credentials, std::string request_body,
      AuthenticationClient::SignInUserCallback callback);

  client::CancellationToken SignInFederated(
      const AuthenticationCredentials& credentials,
      const FederatedSignInType& type,
      const AuthenticationClient::FederatedProperties& properties,
      const AuthenticationClient::SignInUserCallback& callback);

  client::CancellationToken SignInRefresh(
      const AuthenticationCredentials& credentials,
      const RefreshProperties& properties, const SignInUserCallback& callback);

  client::CancellationToken AcceptTerms(
      const AuthenticationCredentials& credentials,
      const std::string& reacceptanceToken, const SignInUserCallback& callback);

  client::CancellationToken SignUpHereUser(
      const AuthenticationCredentials& credentials,
      const AuthenticationClient::SignUpProperties& properties,
      const AuthenticationClient::SignUpCallback& callback);

  client::CancellationToken SignOut(
      const AuthenticationCredentials& credentials,
      const std::string& userAccessToken, const SignOutUserCallback& callback);

  void SetNetworkProxySettings(
      const http::NetworkProxySettings& proxy_settings);

  void SetNetwork(std::shared_ptr<http::Network> network);

  void SetTaskScheduler(std::shared_ptr<TaskScheduler> task_scheduler);

 private:
  using TimeResponse = client::ApiResponse<time_t, client::ApiError>;
  using TimeCallback = std::function<void(TimeResponse)>;

  client::CancellationToken GetTimeFromServer(TimeCallback callback);
  static TimeResponse ParseTimeResponse(std::stringstream& payload);

  std::string base64Encode(const std::vector<uint8_t>& vector);

  std::string generateHeader(const AuthenticationCredentials& credentials,
                             const std::string& url,
                             const time_t& timestamp = std::time(nullptr));
  std::string generateBearerHeader(const std::string& bearer_token);

  http::NetworkRequest::RequestBodyType generateClientBody(
      const SignInProperties& properties);
  http::NetworkRequest::RequestBodyType generateUserBody(
      const AuthenticationClient::UserProperties& properties);
  http::NetworkRequest::RequestBodyType generateFederatedBody(
      const FederatedSignInType,
      const AuthenticationClient::FederatedProperties& properties);
  http::NetworkRequest::RequestBodyType generateRefreshBody(
      const AuthenticationClient::RefreshProperties& properties);
  http::NetworkRequest::RequestBodyType generateSignUpBody(
      const SignUpProperties& properties);
  http::NetworkRequest::RequestBodyType generateAcceptTermBody(
      const std::string& reacceptance_token);

  std::string generateUid();

  client::CancellationToken HandleUserRequest(
      const AuthenticationCredentials& credentials, const std::string& endpoint,
      const http::NetworkRequest::RequestBodyType& request_body,
      const AuthenticationClient::SignInUserCallback& callback);

 private:
  std::shared_ptr<SignInCacheType> client_token_cache_;
  std::shared_ptr<SignInUserCacheType> user_token_cache_;
  AuthenticationSettings settings_;

  mutable std::mutex token_mutex_;
};

void AuthenticationClient::Impl::SetNetworkProxySettings(
    const http::NetworkProxySettings& proxy_settings) {
  settings_.network_proxy_settings = proxy_settings;
}

void AuthenticationClient::Impl::SetNetwork(
    std::shared_ptr<http::Network> network) {
  if (settings_.network_request_handler != network) {
    settings_.network_request_handler = std::move(network);
  }
}

void AuthenticationClient::Impl::SetTaskScheduler(
    std::shared_ptr<TaskScheduler> task_scheduler) {
  if (settings_.task_scheduler != task_scheduler) {
    settings_.task_scheduler = std::move(task_scheduler);
  }
}

AuthenticationClient::Impl::Impl(const std::string& authentication_server_url,
                                 size_t token_cache_limit)
    : client_token_cache_(std::make_shared<SignInCacheType>(token_cache_limit)),
      user_token_cache_(
          std::make_shared<SignInUserCacheType>(token_cache_limit)) {
  settings_.token_endpoint_url = authentication_server_url;
  settings_.token_cache_limit = token_cache_limit;
}

AuthenticationClient::Impl::Impl(AuthenticationSettings settings)
    : client_token_cache_(
          std::make_shared<SignInCacheType>(settings.token_cache_limit)),
      user_token_cache_(
          std::make_shared<SignInUserCacheType>(settings.token_cache_limit)),
      settings_(std::move(settings)) {}

client::CancellationToken AuthenticationClient::Impl::SignInClient(
    AuthenticationCredentials credentials, SignInProperties properties,
    AuthenticationClient::SignInClientCallback callback) {
  if (!settings_.network_request_handler) {
    ExecuteOrSchedule(settings_.task_scheduler, [callback] {
      AuthenticationError result({static_cast<int>(http::ErrorCode::IO_ERROR),
                                  "Cannot sign in while offline"});
      callback(result);
    });
    return client::CancellationToken();
  }
  std::weak_ptr<http::Network> weak_network(settings_.network_request_handler);

  auto cache = client_token_cache_;

  client::CancellationContext context;

  auto time_callback = [=](TimeResponse response) mutable {
    // As a fallback use local system time as a timestamp
    time_t timestamp =
        response.IsSuccessful() ? response.GetResult() : std::time(nullptr);

    std::string url = settings_.token_endpoint_url;
    url.append(kOauthEndpoint);
    http::NetworkSettings network_settings;
    if (settings_.network_proxy_settings) {
      network_settings.WithProxySettings(
          settings_.network_proxy_settings.get());
    }
    http::NetworkRequest request(url);
    request.WithVerb(http::NetworkRequest::HttpVerb::POST);
    request.WithHeader(http::kAuthorizationHeader,
                       generateHeader(credentials, url, timestamp));
    request.WithHeader(http::kContentTypeHeader, kApplicationJson);
    request.WithHeader(http::kUserAgentHeader, http::kOlpSdkUserAgent);
    request.WithSettings(std::move(network_settings));

    std::shared_ptr<std::stringstream> payload =
        std::make_shared<std::stringstream>();
    request.WithBody(generateClientBody(properties));

    auto network_callback = [callback, payload, credentials, cache](
                                const http::NetworkResponse& network_response) {
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

        auto cached_response_found =
            cache->locked([credentials, callback](
                              utils::LruCache<std::string, SignInResult>& c) {
              auto it = c.Find(credentials.GetKey());
              if (it != c.end()) {
                SignInClientResponse response(it->value());
                callback(response);
                return true;
              }
              return false;
            });

        if (!cached_response_found) {
          // Return an error response
          SignInClientResponse error_response(
              AuthenticationError(response_status, error_msg));
          callback(error_response);
        }

        return;
      }

      auto document = std::make_shared<rapidjson::Document>();
      rapidjson::IStreamWrapper stream(*payload);
      document->ParseStream(stream);

      std::shared_ptr<SignInResultImpl> resp_impl =
          std::make_shared<SignInResultImpl>(response_status, error_msg,
                                             document);
      SignInResult response(resp_impl);

      if (response_status == http::HttpStatusCode::OK) {
        // Cache the response
        cache->locked([credentials, &response](
                          utils::LruCache<std::string, SignInResult>& c) {
          return c.InsertOrAssign(credentials.GetKey(), response);
        });
      }
      callback(response);
    };

    context.ExecuteOrCancelled(
        [&]() {
          auto send_outcome = settings_.network_request_handler->Send(
              request, payload, network_callback);
          if (!send_outcome.IsSuccessful()) {
            std::string error_message =
                ErrorCodeToString(send_outcome.GetErrorCode());
            callback(AuthenticationError(
                static_cast<int>(send_outcome.GetErrorCode()), error_message));
            return client::CancellationToken();
          }
          auto request_id = send_outcome.GetRequestId();
          return client::CancellationToken([weak_network, request_id]() {
            auto network = weak_network.lock();

            if (network) {
              network->Cancel(request_id);
            }
          });
        },
        [&]() {
          callback(AuthenticationError(
              static_cast<int>(http::ErrorCode::CANCELLED_ERROR), "Cancelled"));
        });
  };

  context.ExecuteOrCancelled(
      [&]() { return GetTimeFromServer(std::move(time_callback)); }, []() {});
  return client::CancellationToken(
      [context]() mutable { context.CancelOperation(); });
}

AuthenticationClient::Impl::TimeResponse
AuthenticationClient::Impl::ParseTimeResponse(std::stringstream& payload) {
  auto document = std::make_shared<rapidjson::Document>();
  rapidjson::IStreamWrapper stream(payload);
  document->ParseStream(stream);

  if (!document->IsObject()) {
    return client::ApiError(client::ErrorCode::InternalFailure,
                            "JSON document root is not an Object type");
  }

  const auto timestamp_it = document->FindMember("timestamp");
  if (timestamp_it == document->MemberEnd() || !timestamp_it->value.IsUint()) {
    return client::ApiError(
        client::ErrorCode::InternalFailure,
        "JSON document must contain timestamp integer field");
  }

  return timestamp_it->value.GetUint();
}

client::CancellationToken AuthenticationClient::Impl::GetTimeFromServer(
    TimeCallback callback) {
  std::weak_ptr<http::Network> weak_network(settings_.network_request_handler);
  std::string url = settings_.token_endpoint_url;
  url.append(kTimestampEndpoint);
  http::NetworkSettings network_settings;
  if (settings_.network_proxy_settings) {
    network_settings.WithProxySettings(settings_.network_proxy_settings.get());
  }
  http::NetworkRequest request(url);
  request.WithVerb(http::NetworkRequest::HttpVerb::GET);
  request.WithHeader(http::kUserAgentHeader, http::kOlpSdkUserAgent);
  request.WithSettings(std::move(network_settings));

  std::shared_ptr<std::stringstream> payload =
      std::make_shared<std::stringstream>();
  auto cache = user_token_cache_;

  auto send_outcome = settings_.network_request_handler->Send(
      request, payload,
      [callback, payload](const http::NetworkResponse& network_response) {
        if (network_response.GetStatus() != http::HttpStatusCode::OK) {
          callback(
              client::ApiError(network_response.GetStatus(), network_response.GetError()));
          return;
        }

        callback(ParseTimeResponse(*payload));
      });

  if (!send_outcome.IsSuccessful()) {
    std::string error_message = ErrorCodeToString(send_outcome.GetErrorCode());
    callback(client::ApiError(static_cast<int>(send_outcome.GetErrorCode()),
                              error_message));
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

client::CancellationToken AuthenticationClient::Impl::SignInHereUser(
    const AuthenticationCredentials& credentials,
    const UserProperties& properties,
    const AuthenticationClient::SignInUserCallback& callback) {
  return HandleUserRequest(credentials, kOauthEndpoint,
                           generateUserBody(properties), callback);
}

client::CancellationToken AuthenticationClient::Impl::SignInFederated(
    AuthenticationCredentials credentials, std::string request_body,
    AuthenticationClient::SignInUserCallback callback) {
  auto payload =
      std::make_shared<std::vector<unsigned char>>(request_body.size());
  std::memcpy(payload->data(), request_body.data(), request_body.size());
  return HandleUserRequest(credentials, kOauthEndpoint, payload, callback);
}

client::CancellationToken AuthenticationClient::Impl::SignInRefresh(
    const AuthenticationCredentials& credentials,
    const RefreshProperties& properties, const SignInUserCallback& callback) {
  return HandleUserRequest(credentials, kOauthEndpoint,
                           generateRefreshBody(properties), callback);
}

client::CancellationToken AuthenticationClient::Impl::SignInFederated(
    const AuthenticationCredentials& credentials,
    const FederatedSignInType& type, const FederatedProperties& properties,
    const AuthenticationClient::SignInUserCallback& callback) {
  return HandleUserRequest(credentials, kOauthEndpoint,
                           generateFederatedBody(type, properties), callback);
}

client::CancellationToken AuthenticationClient::Impl::AcceptTerms(
    const AuthenticationCredentials& credentials,
    const std::string& reacceptanceToken, const SignInUserCallback& callback) {
  return HandleUserRequest(credentials, kTermsEndpoint,
                           generateAcceptTermBody(reacceptanceToken), callback);
}

client::CancellationToken AuthenticationClient::Impl::HandleUserRequest(
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
  request.WithHeader(http::kAuthorizationHeader,
                     generateHeader(credentials, url));
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

        switch (response_status) {
          case http::HttpStatusCode::OK: {
            // Cache the response
            cache->locked(
                [credentials,
                 &response](utils::LruCache<std::string, SignInUserResult>& c) {
                  return c.InsertOrAssign(credentials.GetKey(), response);
                });
          }
          default:
            callback(response);
            break;
        }
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

client::CancellationToken AuthenticationClient::Impl::SignUpHereUser(
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
  request.WithHeader(http::kAuthorizationHeader,
                     generateHeader(credentials, url));
  request.WithHeader(http::kContentTypeHeader, kApplicationJson);
  request.WithHeader(http::kUserAgentHeader, http::kOlpSdkUserAgent);
  request.WithSettings(std::move(network_settings));

  std::shared_ptr<std::stringstream> payload =
      std::make_shared<std::stringstream>();
  request.WithBody(generateSignUpBody(properties));
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

client::CancellationToken AuthenticationClient::Impl::SignOut(
    const AuthenticationCredentials& credentials,
    const std::string& userAccessToken, const SignOutUserCallback& callback) {
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
                     generateBearerHeader(userAccessToken));
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

std::string AuthenticationClient::Impl::base64Encode(
    const std::vector<uint8_t>& vector) {
  std::string ret = olp::utils::Base64Encode(vector);
  // Base64 encode sometimes return multiline with garbage at the end
  if (!ret.empty()) {
    auto loc = ret.find(kLineFeed);
    if (loc != std::string::npos) ret = ret.substr(0, loc);
  }
  return ret;
}

std::string AuthenticationClient::Impl::generateHeader(
    const AuthenticationCredentials& credentials, const std::string& url,
    const time_t& timestamp) {
  std::string uid = generateUid();
  const std::string currentTime = std::to_string(timestamp);
  const std::string encodedUri = Url::Encode(url);
  const std::string encodedQuery = Url::Encode(
      kOauthConsumerKey + kParamEquals + credentials.GetKey() + kParamAdd +
      kOauthNonce + kParamEquals + uid + kParamAdd + kOauthSignatureMethod +
      kParamEquals + kHmac + kParamAdd + kOauthTimestamp + kParamEquals +
      currentTime + kParamAdd + kOauthVersion + kParamEquals + kVersion);
  const std::string signatureBase =
      kOauthPost + kParamAdd + encodedUri + kParamAdd + encodedQuery;
  const std::string encodeKey = credentials.GetSecret() + kParamAdd;
  auto hmacResult = Crypto::hmac_sha256(encodeKey, signatureBase);
  auto signature = base64Encode(hmacResult);
  std::string authorization =
      "OAuth " + kOauthConsumerKey + kParamEquals + kParamQuote +
      Url::Encode(credentials.GetKey()) + kParamQuote + kParamComma +
      kOauthNonce + kParamEquals + kParamQuote + Url::Encode(uid) +
      kParamQuote + kParamComma + kOauthSignatureMethod + kParamEquals +
      kParamQuote + kHmac + kParamQuote + kParamComma + kOauthTimestamp +
      kParamEquals + kParamQuote + Url::Encode(currentTime) + kParamQuote +
      kParamComma + kOauthVersion + kParamEquals + kParamQuote + kVersion +
      kParamQuote + kParamComma + kOauthSignature + kParamEquals + kParamQuote +
      Url::Encode(signature) + kParamQuote;

  return authorization;
}

std::string AuthenticationClient::Impl::generateBearerHeader(
    const std::string& bearer_token) {
  std::string authorization = http::kBearer + std::string(" ");
  authorization += bearer_token;
  return authorization;
}

http::NetworkRequest::RequestBodyType
AuthenticationClient::Impl::generateClientBody(
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
  return std::make_shared<std::vector<unsigned char> >(
      content, content + data.GetSize());
}

http::NetworkRequest::RequestBodyType
AuthenticationClient::Impl::generateUserBody(const UserProperties& properties) {
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
  return std::make_shared<std::vector<unsigned char> >(
      content, content + data.GetSize());
}

http::NetworkRequest::RequestBodyType
AuthenticationClient::Impl::generateFederatedBody(
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
  return std::make_shared<std::vector<unsigned char> >(
      content, content + data.GetSize());
}

http::NetworkRequest::RequestBodyType
AuthenticationClient::Impl::generateRefreshBody(
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
  return std::make_shared<std::vector<unsigned char> >(
      content, content + data.GetSize());
}

http::NetworkRequest::RequestBodyType
AuthenticationClient::Impl::generateSignUpBody(
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
  return std::make_shared<std::vector<unsigned char> >(
      content, content + data.GetSize());
}

http::NetworkRequest::RequestBodyType
AuthenticationClient::Impl::generateAcceptTermBody(
    const std::string& reacceptance_token) {
  rapidjson::StringBuffer data;
  rapidjson::Writer<rapidjson::StringBuffer> writer(data);
  writer.StartObject();

  writer.Key(kTermsReacceptanceToken);
  writer.String(reacceptance_token.c_str());

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<std::vector<unsigned char> >(
      content, content + data.GetSize());
}

std::string AuthenticationClient::Impl::generateUid() {
  std::lock_guard<std::mutex> lock(token_mutex_);
  {
    static boost::uuids::random_generator gen;

    return boost::uuids::to_string(gen());
  }
}

AuthenticationClient::AuthenticationClient(
    const std::string& authenticationServerUrl, size_t tokenCacheLimit)
    : impl_(std::make_unique<Impl>(authenticationServerUrl, tokenCacheLimit)) {}

AuthenticationClient::AuthenticationClient(AuthenticationSettings settings)
    : impl_(std::make_unique<Impl>(std::move(settings))) {}

AuthenticationClient::~AuthenticationClient() = default;

client::CancellationToken AuthenticationClient::SignInClient(
    const AuthenticationCredentials& credentials,
    const SignInClientCallback& callback,
    const std::chrono::seconds& expires_in) {
  SignInProperties properties;
  properties.expires_in = expires_in;
  return impl_->SignInClient(credentials, properties, callback);
}

client::CancellationToken AuthenticationClient::SignInClient(
    AuthenticationCredentials credentials, SignInProperties properties,
    SignInClientCallback callback) {
  return impl_->SignInClient(std::move(credentials), std::move(properties),
                             std::move(callback));
}

client::CancellationToken AuthenticationClient::SignInHereUser(
    const AuthenticationCredentials& credentials,
    const UserProperties& properties, const SignInUserCallback& callback) {
  return impl_->SignInHereUser(credentials, properties, callback);
}

client::CancellationToken AuthenticationClient::SignInFederated(
    AuthenticationCredentials credentials, std::string request_body,
    SignInUserCallback callback) {
  return impl_->SignInFederated(std::move(credentials), std::move(request_body),
                                std::move(callback));
}

client::CancellationToken AuthenticationClient::SignInFacebook(
    const AuthenticationCredentials& credentials,
    const FederatedProperties& properties, const SignInUserCallback& callback) {
  return impl_->SignInFederated(
      credentials, FederatedSignInType::FacebookSignIn, properties, callback);
}

client::CancellationToken AuthenticationClient::SignInGoogle(
    const AuthenticationCredentials& credentials,
    const FederatedProperties& properties, const SignInUserCallback& callback) {
  return impl_->SignInFederated(credentials, FederatedSignInType::GoogleSignIn,
                                properties, callback);
}

client::CancellationToken AuthenticationClient::SignInArcGis(
    const AuthenticationCredentials& credentials,
    const FederatedProperties& properties, const SignInUserCallback& callback) {
  return impl_->SignInFederated(credentials, FederatedSignInType::ArcgisSignIn,
                                properties, callback);
}

client::CancellationToken AuthenticationClient::SignInRefresh(
    const AuthenticationCredentials& credentials,
    const RefreshProperties& properties, const SignInUserCallback& callback) {
  return impl_->SignInRefresh(credentials, properties, callback);
}

client::CancellationToken AuthenticationClient::SignUpHereUser(
    const AuthenticationCredentials& credentials,
    const SignUpProperties& properties, const SignUpCallback& callback) {
  return impl_->SignUpHereUser(credentials, properties, callback);
}

client::CancellationToken AuthenticationClient::AcceptTerms(
    const AuthenticationCredentials& credentials,
    const std::string& reacceptance_token, const SignInUserCallback& callback) {
  return impl_->AcceptTerms(credentials, reacceptance_token, callback);
}

client::CancellationToken AuthenticationClient::SignOut(
    const AuthenticationCredentials& credentials,
    const std::string& user_access_token, const SignOutUserCallback& callback) {
  return impl_->SignOut(credentials, user_access_token, callback);
}

void AuthenticationClient::SetNetworkProxySettings(
    const http::NetworkProxySettings& proxy_settings) {
  impl_->SetNetworkProxySettings(proxy_settings);
}

void AuthenticationClient::SetNetwork(std::shared_ptr<http::Network> network) {
  impl_->SetNetwork(network);
}

void AuthenticationClient::SetTaskScheduler(
    std::shared_ptr<TaskScheduler> task_scheduler) {
  impl_->SetTaskScheduler(task_scheduler);
}

}  // namespace authentication
}  // namespace olp
