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

#include <boost/optional.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/http/NetworkProxySettings.h>

#include "AuthenticationApi.h"
#include "AuthenticationCredentials.h"
#include "AuthenticationError.h"
#include "SignInResult.h"
#include "SignInUserResult.h"
#include "SignOutResult.h"
#include "SignUpResult.h"

/**
 * @brief The olp namespace
 */
namespace olp {

namespace http {
class Network;
}

namespace thread {
class TaskScheduler;
}

/**
 * @brief The authentication namespace
 */
namespace authentication {

static const std::string kHereAccountProductionUrl =
    "https://account.api.here.com";

/**
 * @brief An API class of the C++ client that provides
 * programmatic access to the HERE Account authentication service.
 *
 * The supported APIs mirror the REST APIs currently available for the HERE
 * Account authentication service.
 */
class AUTHENTICATION_API AuthenticationClient {
 public:
  /**
   * @brief General properties used to sign in with client credentials.
   */
  struct SignInProperties {
    /**
     * @brief (Optional) The scope assigned to the access token.
     */
    boost::optional<std::string> scope{boost::none};

    /**
     * @brief (Optional) The number of seconds left before the access
     * token expires.
     *
     * It must be equal to or more than zero. Ignored if it is zero or greater
     * than the default expiration time supported by the access token endpoint.
     */
    std::chrono::seconds expires_in{0};
  };

  /**
   * @brief The user sign-in properties struct.
   */
  struct UserProperties {
    /**
     * @brief (Required) Your valid email address.
     */
    std::string email;

    /**
     * @brief (Required) Your plain-text password.
     */
    std::string password;

    /**
     * @brief (Optional) The number of seconds left before the access
     * token expires.
     *
     * It must be equal to or more than zero. Ignored if it is zero or greater
     * than the default expiration time of the application.
     */
    unsigned int expires_in = 0;
  };

  /**
   * @brief The federated (Facebook, Google, ArcGIS) sign-in properties
   * structure.
   */
  struct FederatedProperties {
    /**
     * @brief (Required) A valid Facebook, Google, or ArcGis access token
     * obtained from the Facebook, Google, or ArcGis token endpoint.
     */
    std::string access_token;

    /**
     * @brief Required for the first time sign-in; optional for
     * the subsequent sign-ins.
     *
     * The country code should be in the ISO 3166-1 alpha-3 format.
     */
    std::string country_code;

    /**
     * @brief Required for the first time sign-in; optional for
     * the ubsequent sign-ins.
     *
     * The language code should be in the ISO 639-1 alpha-2 format.
     */
    std::string language;

    /**
     * @brief Required for the first time sign-in and if your access token
     * doesn't have the email permission; optional for the subsequent sign-ins.
     */
    std::string email;

    /**
     * @brief (Optional) The number of seconds left before the access
     * token expires.
     *
     * It must be equal to or more than zero. Ignored if it is
     * zero or greater than the default expiration time supported by
     * the application.
     */
    unsigned int expires_in = 0;
  };

  /**
   * @brief Used to create a new HERE account for the specified user
   * with the email and password that are your login credentials.
   */
  struct SignUpProperties {
    /**
     * @brief (Required) Your valid email address.
     */
    std::string email;

    /**
     * @brief (Required) Your plain-text password.
     */
    std::string password;

    /**
     * @brief (Required) Your date of birth in the following format: dd/mm/yyyy.
     */
    std::string date_of_birth;

    /**
     * @brief (Required) Your first name.
     */
    std::string first_name;

    /**
     * @brief (Required) Your last name.
     */
    std::string last_name;

    /**
     * @brief (Required) The code of the country in which you live
     * in the ISO 3166-1 alpha-3 format.
     */
    std::string country_code;

    /**
     * @brief (Required) The code of the language that you speak
     * in the ISO 639-1 alpha-2 format.
     */
    std::string language;

    /**
     * @brief (Optional) Indicates if the user has opted in to marketing.
     */
    bool marketing_enabled = false;

    /**
     * @brief (Optional) Your valid phone number.
     *
     * It must start with the plus (+) sign and consist of 7 to 17 numbers.
     */
    std::string phone_number;

    /**
     * @brief (Optional) The realm in which you want to create the user.
     *
     * The HERE Account realms partition the account data into namespaces.
     * In other words, if you sign up for an account in realm A, you
     * cannot use the same credentials to sign in to realm B. The default realm
     * value is `HERE`.
     */
    std::string realm;

    /**
     * @brief (Optional) The valid Authorization Invite Token with a payload
     * that matches user email and the requested realm. Required for
     * the invite-only realms.
     */
    std::string invite_token;
  };

  /**
   * @brief Used to generate a new access token and contains token values
   * returned as a response to the user sign-in operation.
   */
  struct RefreshProperties {
    /**
     * @brief (Required) The access token value returned as a response
     * to the user sign-in operation.
     *
     * Must match the refresh token.
     */
    std::string access_token;

    /**
     * @brief (Required) The refresh token value returned in the response of
     * the user sign-in operation.
     *
     * Must match the access token.
     */
    std::string refresh_token;

    /**
     * @brief (Optional) The number of seconds left before the access
     * token expires.
     *
     * It must be equal to or more than zero. Ignored if it is
     * zero or greater than the default expiration time supported by
     * the application.
     */
    unsigned int expires_in = 0;
  };

  /**
   * @brief Creates the `AuthenticationClient` instance.
   *
   * @param authentication_server_url The URL of the HERE Account server.
   * @example "https://account.api.here.com
   * @param token_cache_limit The maximum number of tokens that can be cached.
   */
  OLP_SDK_DEPRECATED("Deprecated. Will be removed in 04.2020")
  AuthenticationClient(
      const std::string& authentication_server_url = kHereAccountProductionUrl,
      size_t token_cache_limit = 100u);

  /**
   * @brief A default destructor.
   */
  virtual ~AuthenticationClient();

  /**
   * @brief Defines the callback signature when the client sign-in request is
   * completed.
   */
  using SignInClientResponse =
      client::ApiResponse<SignInResult, AuthenticationError>;
  /**
   * @brief Called when the client sign-in request is completed.
   */
  using SignInClientCallback =
      std::function<void(const SignInClientResponse& response)>;

  /**
   * @brief Defines the callback signature when the user sign-in request is
   * completed.
   */
  using SignInUserResponse =
      client::ApiResponse<SignInUserResult, AuthenticationError>;
  /**
   * @brief Called when the user sign-in request is completed.
   */
  using SignInUserCallback =
      std::function<void(const SignInUserResponse& response)>;

  /**
   * @brief Defines the callback signature when the user sign-up request is
   * completed.
   */
  using SignUpResponse = client::ApiResponse<SignUpResult, AuthenticationError>;
  /**
   * @brief Called when the user sign-up request is completed.
   */
  using SignUpCallback = std::function<void(const SignUpResponse& response)>;

  /**
   * @brief Defines the callback signature when the user sign-out request is
   * completed.
   */
  using SignOutUserResponse =
      client::ApiResponse<SignOutResult, AuthenticationError>;
  /**
   * @brief Called when the user sign-out request is completed.
   */
  using SignOutUserCallback =
      std::function<void(const SignOutUserResponse& response)>;

  /**
   * @brief Deprecated. Use the `SignInClient` method that has the `properties`
   * parameter.
   *
   * Signs in with your HERE Account client credentials. Requests the client
   * access token. Client access tokens cannot be refreshed. Instead,
   * request a new client access token using your client credentials.
   *
   * @param credentials The `AuthenticationCredentials` instance.
   * @param callback The`SignInClientCallback` method that is called when
   * the client sign-in request is completed. If successful, the returned HTTP
   * status is 200. Otherwise, check the response error.
   * @param expires_in (Optional) The number of seconds left before the access
   * token expires. Ignored if it is zero or greater than the default expiration
   * time supported by the application.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the equest.
   */
  OLP_SDK_DEPRECATED(
      "Deprecated. Please use version with properties parameter. Will be "
      "removed by 03.2020")
  client::CancellationToken SignInClient(
      const AuthenticationCredentials& credentials,
      const SignInClientCallback& callback,
      const std::chrono::seconds& expires_in = std::chrono::seconds(0));

  /**
   * @brief Signs in with your HERE Account client credentials and reuests
   * the client access token.
   *
   * The client access tokens cannot be refreshed. Instead
   * request a new client access token using your client credentials.
   *
   * @param credentials The `AuthenticationCredentials` instance.
   * @param properties The `SignInProperties` structure that has the scope and
   * expiration time.
   * @param callback The`SignInClientCallback` method that is called when
   * the client sign-in request is completed. If successful, the returned HTTP
   * status is 200. Otherwise, check the response error.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken SignInClient(AuthenticationCredentials credentials,
                                         SignInProperties properties,
                                         SignInClientCallback callback);

  /**
   * @brief Signs in with the email and password that you used for
   * registration via the sign-up API and requests your user access token.
   *
   * The user access tokens can be refreshed using the `SignInRefresh` method.
   *
   * @param credentials The `AuthenticationCredentials` instance.
   * @param properties The `UserProperties` structure.
   * @param callback The `SignInUserCallback` method that is called when
   * the user sign-in request is completed. If successful, the returned HTTP
   * status is 200. Otherwise, check the response error.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken SignInHereUser(
      const AuthenticationCredentials& credentials,
      const UserProperties& properties, const SignInUserCallback& callback);

  /**
   * @brief Signs in with a custom request body.
   *
   * This method provides the mechanism to authenticate with the HERE Platform
   * using a custom request body. You should use this method when the HERE
   * Platform authentication backend offers you individual parameters or
   * endpoint.
   *
   * @param credentials The `AuthenticationCredentials` instance.
   * @param request_body The request body that will be passed to the oauth
   * endpoint.
   * @param callback The `SignInUserCallback` method that is called when
   * the user sign-in request is completed. If successful, the returned HTTP
   * status is 200. If a new account is created as a part of the sign-in request
   * and terms must be accepted, the returned HTTP status is 201  for the first
   * time and 412 for subsequent requests as long as you accept the terms.
   * Otherwise, check the response error.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken SignInFederated(
      AuthenticationCredentials credentials, std::string request_body,
      SignInUserCallback callback);

  /**
   * @brief Signs in with your valid Facebook token and requests your user
   * access token.
   *
   * If this is the first time that you use Facebook to sign in, a new HERE
   * Account is automatically created and filled in with the data from your
   * Facebook account, including your name and email.
   *
   * @param credentials The `AuthenticationCredentials` instance.
   * @param properties The `FederatedProperties` structure.
   * @param callback The `SignInUserCallback` method that is called when
   * the user sign-in request is completed. If successful, the returned HTTP
   * status is 200. If a new account is created as a part of the sign-in request
   * and terms must be accepted, the returned HTTP status is 201.
   * Otherwise, check the response error.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken SignInFacebook(
      const AuthenticationCredentials& credentials,
      const FederatedProperties& properties,
      const SignInUserCallback& callback);
  /**
   * @brief Signs in with your valid Google token and requests your user access
   * token.
   *
   * If this is the first time that you use Google to sign in,
   * a new HERE Account is automatically created and filled in with the data
   * from your Google account, including your name and email.
   *
   * @param credentials The `AuthenticationCredentials` instance.
   * @param properties The `FederatedProperties` structure.
   * @param callback The `SignInUserCallback` method that is called when
   * the user sign-in request is completed. If successful, the returned HTTP
   * status is 200. If a new account is created as a part of the sign-in request
   * and terms must be accepted, the returned HTTP status is 201. Otherwise,
   * check the response error.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken SignInGoogle(
      const AuthenticationCredentials& credentials,
      const FederatedProperties& properties,
      const SignInUserCallback& callback);
  /**
   * @brief Signs in with your valid ArcGIS token and requests your user access
   * token.
   *
   * If this is the first time that you use ArcGIS to sign in,
   * a new HERE Account is automatically created and filled in with the data
   * from your ArcGIS account, including your name and email.
   *
   * @param credentials The `AuthenticationCredentials` instance.
   * @param properties The `FederatedProperties` structure.
   * @param callback The `SignInUserCallback` method that is called when
   * the user sign-in request is completed. If successful, the returned HTTP
   * status is 200. If a new account is created as a part of the sign-in request
   * and terms must be accepted, the returned HTTP status is 201. Otherwise,
   * check the response error.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken SignInArcGis(
      const AuthenticationCredentials& credentials,
      const FederatedProperties& properties,
      const SignInUserCallback& callback);
  /**
   * @brief Signs in with the refresh token.
   *
   * Exchanges the user access token and refresh token for a new user access
   * token. The HERE user access token expires in 24 hours. Not to ask to
   * specify the credentials again, a new access token can be requested using
   * the refresh token. The refresh token is always matched to a particular
   * access token and must be kept secure in the client. The access token can
   * already be expired when used together with the refresh token. The refresh
   * token expires after being used. There is a limit of 3 simultaneously active
   * refresh tokens per user. After logging in 4 times, the first token-pair can
   * no longer be refreshed.
   *
   * @param credentials The `AuthenticationCredentials` instance.
   * @param properties The `RefreshProperties` structure.
   * @param callback The `SignInUserCallback` method that is called when
   * the user sign-in request is completed. If successful, the returned HTTP
   * status is 200. Otherwise, check the response error.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken SignInRefresh(
      const AuthenticationCredentials& credentials,
      const RefreshProperties& properties, const SignInUserCallback& callback);

  /**
   * @brief Signs up with your user properties and creates a new HERE Account
   * for using your email and password that are the login credentials.
   *
   * The HERE user is uniquely identified by the user ID that is consistent
   * across the other HERE services, regardless of the authentication method
   * used.
   *
   * @param credentials The `AuthenticationCredentials` instance.
   * @param properties The `SignUpProperties` structure.
   * @param callback The `SignUpCallback` method that is called when the user
   * sign-up request is completed. If successful, the returned HTTP status
   * is 201. Otherwise, check the response error.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken SignUpHereUser(
      const AuthenticationCredentials& credentials,
      const SignUpProperties& properties, const SignUpCallback& callback);
  /**
   * @brief Accepts the terms and conditions.
   *
   * Requires the "terms re-acceptance required" response represented by
   * the following statuses:
   * * HTTP 412 &ndash; received after you sign in with the existing user
   * account.
   * * HTTP 201 &ndash; received after you create a new account.
   * The terms and conditions are explicitly accepted by providing the terms
   * re-acceptance token back to this API.
   *
   * @param credentials The `AuthenticationCredentials` instance.
   * @param reacceptance_token The terms re-acceptance token from the
   * HTTP 412 or HTTP 201 response:
   * `SignInUserResponse::term_acceptance_token()`.
   * @param callback The `SignInUserCallback` method that is called when
   * the user sign-in request is completed. If successful, the returned HTTP
   * status is 204. Otherwise, check the response error.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken AcceptTerms(
      const AuthenticationCredentials& credentials,
      const std::string& reacceptance_token,
      const SignInUserCallback& callback);

  /**
   * @brief Signs out the user.
   *
   * Calls the sign-out API and deletes any locally stored tokens.
   * The access token that is used to sign out is immediately invalidated for
   * the token refresh purposes.
   * @note This is not a global sign-out. Other devices or services that have
   * different access tokens for the same user remain signed in.
   *
   * @param credentials The `AuthenticationCredentials` instance.
   * @param user_access_token The access token from the `SignInUserResponse`
   * instance.
   * @param callback The `SignOutUserCallback` method that is called when
   * the user sign-out request is completed. If successful, the returned HTTP
   * status is 204. Otherwise, check the response error.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken SignOut(
      const AuthenticationCredentials& credentials,
      const std::string& user_access_token,
      const SignOutUserCallback& callback);

  /**
   * @brief Sets the `NetworkProxySettings` instance used by the underlying
   * network layer to make requests.
   *
   * It is recommended to call this function before you
   * make any requests using the `AuthenticationClient` instance.
   *
   * @param proxy_settings The proxy settings used by the network layer to make
   * requests.
   */
  OLP_SDK_DEPRECATED("Deprecated. Will be removed in 04.2020")
  void SetNetworkProxySettings(
      const http::NetworkProxySettings& proxy_settings);

  /**
   * @brief Sets the `Network` class handler used to make requests.
   *
   * @param network The shared `Network` instance used to trigger requests.
   * Should not be `nullptr`; otherwise, any access to the class method calls
   * would render `http::ErrorCode::IO_ERROR`.
   */
  OLP_SDK_DEPRECATED("Deprecated. Will be removed in 04.2020")
  void SetNetwork(std::shared_ptr<http::Network> network);

  /**
   * @brief Sets the `TaskScheduler` class.
   *
   * @param task_scheduler The shared `TaskScheduler` instance used for requests
   * outcomes. If set to `nullptr`, callbacks are sent synchronously.
   */
  OLP_SDK_DEPRECATED("Deprecated. Will be removed in 04.2020")
  void SetTaskScheduler(std::shared_ptr<thread::TaskScheduler> task_scheduler);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace authentication
}  // namespace olp
