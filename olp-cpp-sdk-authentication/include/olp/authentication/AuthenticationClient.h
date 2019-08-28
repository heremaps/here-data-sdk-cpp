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

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "AuthenticationApi.h"
#include "AuthenticationCredentials.h"
#include "AuthenticationError.h"
#include "NetworkProxySettings.h"
#include "SignInResult.h"
#include "SignInUserResult.h"
#include "SignOutResult.h"
#include "SignUpResult.h"

#include "olp/core/client/ApiResponse.h"
#include "olp/core/client/CancellationToken.h"

/**
 * @brief olp namespace
 */
namespace olp {
/**
 * @brief authentication namespace
 */
namespace authentication {

static const std::string kHereAccountProductionUrl =
    "https://account.api.here.com";

/**
 * @brief AuthenticationClient is a C++ client API class that provides
 * programatic access to HERE Account authentication service. The supported APIs
 * mirror the REST APIs currently available for HERE Account authentication
 * service.
 */
class AUTHENTICATION_API AuthenticationClient {
 public:
  /**
   * @brief The password user sign-in properties struct.
   */
  struct UserProperties {
    /**
     * @brief email Required. A valid email address.
     */
    std::string email;

    /**
     * @brief password. Required. Plain-text password.
     */
    std::string password;

    /**
     * @brief expires_in Optional. Number of seconds before token expires, must
     * number zero or more. Ignored if zero or greater than default expiration
     * of the application.
     */
    unsigned int expires_in = 0;
  };

  /**
   * @brief The federated (Facebook, Google, ArcGIS) sign-in properties struct.
   */
  struct FederatedProperties {
    /**
     * @brief access_token A valid Facebook, Google or ArcGis access token,
     * obtained from Facebook, Google or ArcGis sign-in is required.
     */
    std::string access_token;

    /**
     * @brief country_code Required for first time sign-in, optional for
     * subsequent sign-ins. ISO 3166-1 alpha-3 country code.
     */
    std::string country_code;

    /**
     * @brief language Required if first time sign-in, optional for subsequent
     * sign-ins. ISO 639-1 2 letter language code.
     */
    std::string language;

    /**
     * @brief email Required for first time sign in and if access token doesn't
     * have email permission. Optional for subsequent sign-ins.
     */
    std::string email;

    /**
     * @brief expires_in Optional. Number of seconds before token expires, must
     * be zero or more. Ignored if zero or if greater than default expiration of
     * the application.
     */
    unsigned int expires_in = 0;
  };

  /**
   * @brief The SignUpProperties struct is used to create a new HERE account for
   * the specified user, with the provided email and password being the login
   * credentials to the signInUser API.
   */
  struct SignUpProperties {
    /**
     * @brief email Required. A valid email address.
     */
    std::string email;

    /**
     * @brief password Required. Plain-text password.
     */
    std::string password;

    /**
     * @brief date_of_birth Required. "dd/mm/yyyy" format.
     */
    std::string date_of_birth;

    /**
     * @brief first_name Required.
     */
    std::string first_name;

    /**
     * @brief last_name Required.
     */
    std::string last_name;

    /**
     * @brief country_code Required. ISO 3166-1 alpha-3 country code.
     */
    std::string country_code;

    /**
     * @brief language Required. ISO 639-1 2 letter language code.
     */
    std::string language;

    /**
     * @brief marketing_enabled Optional. Indicates if the user has opted in to
     * marketing.
     */
    bool marketing_enabled = false;

    /**
     * @brief phone_number Valid phone numbers consist of 7 to 17 numbers, with
     * a leading plus
     * ('+') sign.
     */
    std::string phone_number;

    /**
     * @brief realm Optional. The realm the user will be created into. The HERE
     * Account realms partition the account data into namespaces. In other
     * words, signing up for an account in realm A, will not automatically allow
     * you to sign in on realm B using the same email and password (or other
     * credentials, such as Facebook tokens). The default realm value is HERE.
     */
    std::string realm;

    /**
     * @brief invite_token Optional. Valid Authorization Invite Token with
     * payload matching email and realm of the request. Required for invite-only
     * realms.
     */
    std::string invite_token;
  };

  /**
   * @brief The RefreshProperties struct is used when generating a new access
   * token and contains token values returned in the response of a user sign in
   * operation.
   */
  struct RefreshProperties {
    /**
     * @brief access_token Required. Access token value returned in the response
     * of a user sign-in operation. Must match the refresh token.
     */
    std::string access_token;

    /**
     * @brief refresh_token Required. Refresh token value returned in the
     * response of a user sign-in operation.  Must match the access token.
     */
    std::string refresh_token;

    /**
     * @brief expires_in Optional. Number of seconds before token expires, must
     * be zero or more. Ignored if zero or if greater than default expiration of
     * the application.
     */
    unsigned int expires_in = 0;
  };

  /**
   * @brief Constructor
   * @param authentication_server_url HERE Account server URL (eg.
   * "https://account.api.here.com")
   * @param token_cache_limit Maximum number of tokens that will be cached.
   */
  AuthenticationClient(
      const std::string& authentication_server_url = kHereAccountProductionUrl,
      size_t token_cache_limit = 100u);

  /**
   * @brief Destructor
   */
  virtual ~AuthenticationClient();

  /**
   * @brief SignInClientCallback defines the callback signature for when the
   * client sign-in request is completed.
   */
  using SignInClientResponse =
      client::ApiResponse<SignInResult, AuthenticationError>;
  using SignInClientCallback =
      std::function<void(const SignInClientResponse& response)>;

  /**
   * @brief SignInUserCallback defines the callback signature for when the user
   * sign-in request is completed.
   */
  using SignInUserResponse =
      client::ApiResponse<SignInUserResult, AuthenticationError>;
  using SignInUserCallback =
      std::function<void(const SignInUserResponse& response)>;

  /**
   * @brief SignUpCallback defines the callback signature for when the user
   * sign-up request is completed.
   */
  using SignUpResponse = client::ApiResponse<SignUpResult, AuthenticationError>;
  using SignUpCallback = std::function<void(const SignUpResponse& response)>;

  /**
   * @brief SignOutUserCallback defines the callback signature for when the user
   * sign-out request is completed.
   */
  using SignOutUserResponse =
      client::ApiResponse<SignOutResult, AuthenticationError>;
  using SignOutUserCallback =
      std::function<void(const SignOutUserResponse& response)>;

  /**
   * @brief Sign in with HERE account client credentials, requests a client
   * access token that identifies the application or service by providing client
   * credentials. Client access tokens can not be refreshed, instead request a
   * new one using client credentials.
   * @param credentials Client access keys issued for the client by the HERE
   * Account as part of the onboarding or support process.
   * @param callback The method to be called when request is completed. In case
   * of successful client sign-in request, the returned HTTP status is 200.
   * Otherwise, check the response error.
   * @param expires_in Optional. The number of seconds until the new access
   * token expires. Ignored if zero or if greater than default expiration of the
   * application.
   * @return CancellationToken that can be used to cancel the request.
   */
  client::CancellationToken SignInClient(
      const AuthenticationCredentials& credentials,
      const SignInClientCallback& callback,
      const std::chrono::seconds& expires_in = std::chrono::seconds(0));

  /**
   * @brief Sign in by providing the user email and password previously
   * registered using the sign-up API, requests a user access token. User access
   * tokens can be refreshed using the SignInRefresh method.
   * @param credentials Client access keys issued for the client by the HERE
   * Account as part of the onboarding or support process.
   * @param properties HERE user properties used when user registered using the
   * sign-up API.
   * @param callback  The method to be called when request is completed. In case
   * of successful user sign-in, the returned HTTP status is 200. Otherwise,
   * check the response error.
   * @return CancellationToken that can be used to cancel the request.
   */
  client::CancellationToken SignInHereUser(
      const AuthenticationCredentials& credentials,
      const UserProperties& properties, const SignInUserCallback& callback);
  /**
   * @brief Sign in by providing a valid Facebook token, requests a user access
   * token. If this is the first time this particular Facebook user is signing
   * in, a new HERE account is automatically created and filled with details
   * from Facebook, including name and email.
   * @param credentials Client access keys issued for the client by the HERE
   * Account as part of the onboarding or support process.
   * @param properties Federated properties used when signing in.
   * @param callback The method to be called when request is completed. In case
   * of successful user sign-in, the returned HTTP status is 200. In case a new
   * account was created as part of the sign in and terms must be accepted,the
   * returned HTTP status is 201. Otherwise, check the response error.
   * @return CancellationToken that can be used to cancel the request.
   */
  client::CancellationToken SignInFacebook(
      const AuthenticationCredentials& credentials,
      const FederatedProperties& properties,
      const SignInUserCallback& callback);
  /**
   * @brief Sign in by providing a valid Google token, requests a user access
   * token. If this is the first time this particular Google user is signing in,
   * a new HERE account is automatically created and filled with details from
   * Google, including name and email.
   * @param credentials Client access keys issued for the client by the HERE
   * Account as part of the onboarding or support process.
   * @param properties Federated properties used when signing in.
   * @param callback The method to be called when request is completed. In case
   * of successful user sign-in, the returned HTTP status is 200. In case a new
   * account was created as part of the sign in and terms must be accepted,the
   * returned HTTP status is 201. Otherwise, check the response error.
   * @return CancellationToken that can be used to cancel the request.
   */
  client::CancellationToken SignInGoogle(
      const AuthenticationCredentials& credentials,
      const FederatedProperties& properties,
      const SignInUserCallback& callback);
  /**
   * @brief Sign in by providing a valid ArcGIS token, requests a user access
   * token. If this is the first time this particular ArcGIS user is signing in,
   * a new HERE account is automatically created and filled with details from
   * ArcGIS, including name and email.
   * @param credentials Client access keys issued for the client by the HERE
   * Account as part of the onboarding or support process.
   * @param properties Federated properties used when signing in.
   * @param callback The method to be called when request is completed. In case
   * of successful user sign-in, the returned HTTP status is 200. In case a new
   * account was created as part of the sign in and terms must be accepted, the
   * returned HTTP status is 201. Otherwise, check the response error.
   * @return CancellationToken that can be used to cancel the request.
   */
  client::CancellationToken SignInArcGis(
      const AuthenticationCredentials& credentials,
      const FederatedProperties& properties,
      const SignInUserCallback& callback);
  /**
   * @brief Sign in with refresh token exchanges a user access token and refresh
   * token for a new user access token. A HERE user access token will typically
   * expire in 24 hours. In order to avoid prompting the user for credentials
   * again, a new access token can be requested by providing a credential
   * substitute called the refresh token. A refresh token is always matched to a
   * particular access token, and must be kept secure in the client. The access
   * token can already be expired when used together with the refresh token. A
   * refresh token expires after being used. There is a limit of 3
   * simultaneously active refresh tokens per user. After logging in 4 times,
   * the first token-pair can no longer be refreshed.
   * @param credentials Client access keys issued for the client by the HERE
   * Account as part of the onboarding or support process.
   * @param properties Properties that will be used in order to refresh token.
   * @param callback  The method to be called when request is completed. In case
   * of sucessful user sign-in, the returned HTTP status is 200. Otherwise,
   * check the response error.
   * @return CancellationToken that can be used to cancel the request.
   */
  client::CancellationToken SignInRefresh(
      const AuthenticationCredentials& credentials,
      const RefreshProperties& properties, const SignInUserCallback& callback);

  /**
   * @brief Sign up with user properties creates a new HERE account for the
   * specified user, with the provided email and password being the login
   * credentials. A HERE user is uniquely identified by the user ID that is
   * consistent across other HERE services, regardless of the authentication
   * method used (password or any federated login, e.g. via Facebook, Google,
   * ArcGIS).
   * @param credentials Client access keys issued for the client by the HERE
   * Account as part of the onboarding or support process.
   * @param properties User properties that will be used in order to create a
   * new HERE account.
   * @param callback  The method to be called when request is completed. In case
   * of a successful user account creation, the returned HTTP status is 201.
   * Otherwise, check the response error.
   * @return CancellationToken that can be used to cancel the request.
   */
  client::CancellationToken SignUpHereUser(
      const AuthenticationCredentials& credentials,
      const SignUpProperties& properties, const SignUpCallback& callback);
  /**
   * @brief Accept terms and conditions after receiving a terms re-acceptance
   * required response represented by HTTP 412 status after signing in with
   * existing user account or HTTP 201 status after creating new account. The
   * terms are explicitly accepted by providing the terms re-acceptance token
   * back to this API.
   * @param credentials Client access keys issued for the client by the HERE
   * Account as part of the onboarding or support process.
   * @param reacceptance_token Required. The terms re-acceptance token from the
   * HTTP 412 or HTTP 201 "terms re-acceptance required" response
   * (SignInUserResponse::term_acceptance_token()).
   * @param callback The method to be called when request is completed. In case
   * of a successful operation, the returned HTTP status is 204. Otherwise,
   * check the response error.
   * @return CancellationToken that can be used to cancel the request.
   */
  client::CancellationToken AcceptTerms(
      const AuthenticationCredentials& credentials,
      const std::string& reacceptance_token,
      const SignInUserCallback& callback);

  /**
   * @brief Sign out the user. In addition to calling the sign out API, the
   * client should delete any locally stored tokens. The access token that is
   * used to sign out is immediately invalidated for token refresh purposes.
   * Note that this is not a global sign-out, other devices or services having
   * different access tokens for the same user remain signed in.
   * @param credentials Client access keys issued for the client by the HERE
   * Account as part of the onboarding or support process.
   * @param user_access_token Access token from SignInUserResponse.
   * @param callback The method to be called when request is completed. In case
   * of a successful user sign-out operation, the returned HTTP status is 204.
   * Otherwise, check the response error.
   * @return CancellationToken that can be used to cancel the request.
   */
  client::CancellationToken SignOut(
      const AuthenticationCredentials& credentials,
      const std::string& user_access_token,
      const SignOutUserCallback& callback);

  /**
   * @brief Apply NetworkProxySettings to be used by the underlying network
   * layer when making requests. It is recommended to call this function before
   * making any requests with a given AuthenticationClient instance.
   * @param proxy_settings proxy settings to be used by the network layer when
   * making requests.
   * @return false if the NetworkProxySettings provided are invalid (e.g.
   * missing host address) or a network connection is currently active. True if
   * the proxy settings are applied succesfully.
   */
  bool SetNetworkProxySettings(const NetworkProxySettings& proxy_settings);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace authentication
}  // namespace olp
