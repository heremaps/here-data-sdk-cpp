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

#pragma once

#include <memory>
#include <string>

#include "olp/authentication/AuthenticationClient.h"
#include "olp/authentication/AuthenticationSettings.h"
#include "olp/authentication/AuthorizeRequest.h"
#include "olp/authentication/Types.h"
#include "olp/core/client/ApiError.h"
#include "olp/core/client/CancellationToken.h"
#include "olp/core/client/ErrorCode.h"
#include "olp/core/client/OlpClient.h"
#include "olp/core/client/PendingRequests.h"
#include "olp/core/http/HttpStatusCode.h"
#include "olp/core/http/NetworkRequest.h"
#include "olp/core/porting/make_unique.h"
#include "olp/core/thread/Atomic.h"
#include "olp/core/utils/LruCache.h"

namespace olp {
namespace authentication {

using SignInProperties = AuthenticationClient::SignInProperties;
using UserProperties = AuthenticationClient::UserProperties;
using FederatedProperties = AuthenticationClient::FederatedProperties;
using SignUpProperties = AuthenticationClient::SignUpProperties;
using RefreshProperties = AuthenticationClient::RefreshProperties;

using SignInClientCallback = AuthenticationClient::SignInClientCallback;
using SignInClientResponse = AuthenticationClient::SignInClientResponse;
using SignInUserCallback = AuthenticationClient::SignInUserCallback;
using SignInUserResponse = AuthenticationClient::SignInUserResponse;
using SignUpCallback = AuthenticationClient::SignUpCallback;
using SignOutUserCallback = AuthenticationClient::SignOutUserCallback;

using TimeResponse = Response<time_t>;
using TimeCallback = Callback<time_t>;

enum class FederatedSignInType { FacebookSignIn, ArcgisSignIn };

class AuthenticationClientImpl {
 public:
  /// The sign in cache alias type
  using SignInCacheType =
      thread::Atomic<utils::LruCache<std::string, SignInResult>>;

  /// The sign in user cache alias type
  using SignInUserCacheType =
      thread::Atomic<utils::LruCache<std::string, SignInUserResult>>;

  static constexpr auto kApplicationJson = "application/json";

  explicit AuthenticationClientImpl(AuthenticationSettings settings);
  virtual ~AuthenticationClientImpl();

  /**
   * @brief Sign in with client credentials
   * @param credentials Client credentials obtained when registering application
   *                    on HERE developer portal.
   * @param callback  The method to be called when request is completed.
   * @param expiresIn The number of seconds until the new access token expires.
   * @return CancellationToken that can be used to cancel the request.
   */
  client::CancellationToken SignInClient(AuthenticationCredentials credentials,
                                         SignInProperties properties,
                                         SignInClientCallback callback);

  client::CancellationToken SignInHereUser(
      const AuthenticationCredentials& credentials,
      const UserProperties& properties, const SignInUserCallback& callback);

  client::CancellationToken SignInFederated(
      AuthenticationCredentials credentials, std::string request_body,
      SignInUserCallback callback);

  client::CancellationToken SignInFederated(
      const AuthenticationCredentials& credentials,
      const FederatedSignInType& type, const FederatedProperties& properties,
      const SignInUserCallback& callback);

  client::CancellationToken SignInApple(AppleSignInProperties properties,
                                        SignInUserCallback callback);

  client::CancellationToken SignInRefresh(
      const AuthenticationCredentials& credentials,
      const RefreshProperties& properties, const SignInUserCallback& callback);

  client::CancellationToken AcceptTerms(
      const AuthenticationCredentials& credentials,
      const std::string& reacceptance_token,
      const SignInUserCallback& callback);

  client::CancellationToken SignUpHereUser(
      const AuthenticationCredentials& credentials,
      const SignUpProperties& properties, const SignUpCallback& callback);

  client::CancellationToken SignOut(
      const AuthenticationCredentials& credentials,
      const std::string& access_token, const SignOutUserCallback& callback);

  client::CancellationToken IntrospectApp(std::string access_token,
                                          IntrospectAppCallback callback);

  client::CancellationToken Authorize(std::string access_token,
                                      AuthorizeRequest request,
                                      AuthorizeCallback callback);

  client::CancellationToken GetMyAccount(std::string access_token,
                                         UserAccountInfoCallback callback);

 protected:
  class RequestTimer {
   public:
    RequestTimer();
    explicit RequestTimer(std::time_t server_time);
    std::time_t GetRequestTime() const;

   private:
    std::chrono::steady_clock::time_point timer_start_;
    std::time_t time_;
  };

  virtual TimeResponse GetTimeFromServer(client::CancellationContext context,
                                         const client::OlpClient& client) const;

  static TimeResponse ParseTimeResponse(std::stringstream& payload);

  client::OlpClient::RequestBodyType GenerateClientBody(
      const SignInProperties& properties);
  client::OlpClient::RequestBodyType GenerateUserBody(
      const UserProperties& properties);
  client::OlpClient::RequestBodyType GenerateFederatedBody(
      const FederatedSignInType, const FederatedProperties& properties);
  client::OlpClient::RequestBodyType GenerateRefreshBody(
      const RefreshProperties& properties);
  client::OlpClient::RequestBodyType GenerateSignUpBody(
      const SignUpProperties& properties);
  client::OlpClient::RequestBodyType GenerateAcceptTermBody(
      const std::string& reacceptance_token);
  client::OlpClient::RequestBodyType GenerateAuthorizeBody(
      const AuthorizeRequest& properties);

  virtual olp::client::HttpResponse CallAuth(
      const client::OlpClient& client, const std::string& endpoint,
      client::CancellationContext context,
      const AuthenticationCredentials& credentials,
      client::OlpClient::RequestBodyType body, std::time_t timestamp,
      const std::string& content_type = kApplicationJson);

  SignInResult ParseAuthResponse(int status, std::stringstream& auth_response);

  SignInUserResult ParseUserAuthResponse(int status,
                                         std::stringstream& auth_response);

  template <typename SignInResponseType>
  Response<SignInResponseType> GetSignInResponse(
      const client::HttpResponse& auth_response,
      const client::CancellationContext& context, const std::string& key);

  template <typename SignInResponseType>
  boost::optional<SignInResponseType> FindInCache(const std::string& key);

  template <typename SignInResponseType>
  void StoreInCache(const std::string& key, SignInResponseType);

  std::string GenerateUid() const;

  client::CancellationToken HandleUserRequest(
      const AuthenticationCredentials& credentials, const std::string& endpoint,
      const client::OlpClient::RequestBodyType& request_body,
      const SignInUserCallback& callback);

  RequestTimer CreateRequestTimer(const client::OlpClient& client,
                                  client::CancellationContext context) const;

  std::shared_ptr<SignInCacheType> client_token_cache_;
  std::shared_ptr<SignInUserCacheType> user_token_cache_;
  AuthenticationSettings settings_;
  std::shared_ptr<client::PendingRequests> pending_requests_;
  mutable std::mutex token_mutex_;
};

}  // namespace authentication
}  // namespace olp
