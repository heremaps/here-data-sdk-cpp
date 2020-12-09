/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include <olp/authentication/AuthenticationApi.h>
#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/AuthenticationError.h>
#include <olp/authentication/AuthenticationSettings.h>
#include <olp/authentication/AuthorizeRequest.h>
#include <olp/authentication/SignInResult.h>
#include <olp/authentication/SignInUserResult.h>
#include <olp/authentication/SignOutResult.h>
#include <olp/authentication/SignUpResult.h>
#include <olp/authentication/Types.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationToken.h>
#include <boost/optional.hpp>

/**
 * @brief Rules all the other namespaces.
 */
namespace olp {

/**
 * @brief Holds all authentication-related classes.
 */
namespace authentication {
class AuthenticationClientImpl;

/**
 * @brief Provides programmatic access to the HERE Account Authentication Service.
 *
 * The supported APIs mirror the REST APIs currently available for the HERE
 * Account Authentication Service.
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
    unsigned int expires_in{0};
  };

  /**
   * @brief The federated (Facebook, Google, ArcGIS) sign-in properties
   * structure.
   */
  struct FederatedProperties {
    /**
     * @brief (Required) A valid Facebook, Google, or ArcGIS access token
     * obtained from the Facebook, Google, or ArcGIS token endpoint.
     */
    std::string access_token;

    /**
     * @brief The code of the country in which you live
     * in the ISO 3166-1 alpha-3 format.
     *
     * Required for the first time sign-in; optional for
     * the subsequent sign-ins.
     */
    std::string country_code;

    /**
     * @brief The code of the language that you speak
     * in the ISO 639-1 alpha-2 format.
     *
     * Required for the first time sign-in; optional for
     * the subsequent sign-ins.
     */
    std::string language;

    /**
     * @brief Your valid email address.
     *
     * Required for the first time sign-in and if your access token
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
    unsigned int expires_in{0};
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
    bool marketing_enabled{false};

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
     * that matches the user email and requested realm. Required for
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
    unsigned int expires_in{0};
  };

  /// The client sign-in response type.
  using SignInClientResponse = Response<SignInResult>;

  /// The callback type of the client sign-in response.
  using SignInClientCallback = Callback<SignInResult>;

  /// The user sign-in response type.
  using SignInUserResponse = Response<SignInUserResult>;

  /// The callback type of the user sign-in response.
  using SignInUserCallback = Callback<SignInUserResult>;

  /// The client sign-up response type.
  using SignUpResponse = Response<SignUpResult>;

  /// The callback type of the user sign-up response.
  using SignUpCallback = Callback<SignUpResult>;

  /// The client sign-out response type.
  using SignOutUserResponse = Response<SignOutResult>;

  /// The callback type of the user sign-out response.
  using SignOutUserCallback = Callback<SignOutResult>;

  /**
   * @brief Creates the `AuthenticationClient` instance.
   *
   * @param settings The authentication settings that can be used to configure
   * the `AuthenticationClient` instance.
   */
  explicit AuthenticationClient(AuthenticationSettings settings);
  virtual ~AuthenticationClient();

  // Non-copyable but movable
  AuthenticationClient(const AuthenticationClient&) = delete;
  AuthenticationClient& operator=(const AuthenticationClient&) = delete;
  AuthenticationClient(AuthenticationClient&&) noexcept;
  AuthenticationClient& operator=(AuthenticationClient&&) noexcept;

  /**
   * @brief Signs in with your HERE Account client credentials and requests
   * the client access token.
   *
   * The client access tokens cannot be refreshed. Instead,
   * request a new client access token using your client credentials.
   *
   * @param credentials The `AuthenticationCredentials` instance.
   * @param properties The `SignInProperties` structure that has the scope
   * and expiration time.
   * @param callback The`SignInClientCallback` method that is called when
   * the client sign-in request is completed. If successful, the returned
   * HTTP status is 200. Otherwise, check the response error.
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
   * This method provides the mechanism to authenticate with the HERE platform
   * using a custom request body. You should use this method when the HERE
   * platform authentication backend offers you individual parameters or
   * endpoint.
   *
   * @param credentials The `AuthenticationCredentials` instance.
   * @param request_body The request body that will be passed to the OAuth
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
   *
   * @deprecated Will be removed by 12.2020.
   */
  OLP_SDK_DEPRECATED(
      "Sign in with Google token is deprecated and will be removed by 12.2020")
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
   * to use your login credentials: email and password.
   *
   * The HERE user is uniquely identified by the user ID that is consistent
   * across the other HERE platform Services, regardless of the authentication
   * method used.
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
   * * HTTP 412 – received after you sign in with the existing user
   * account.
   * * HTTP 201 – received after you create a new account.
   *
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
   * @brief Signs the user out.
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
   * @brief Retrieves the application associated with the client access token.
   *
   * The application does not need permissions to access this endpoint.
   * It will allow any client access token to retrieve its own information.
   *
   * @param access_token A valid access token that is associated with the
   * application.
   * @param callback The`IntrospectAppCallback` method that is called when
   * the client introspect request is completed.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken IntrospectApp(std::string access_token,
                                          IntrospectAppCallback callback);

  /**
   * @brief Retrieves policy decision for a given request context against the
   * HERE Service.
   *
   * Collects all permissions associated with the authenticated user or
   * application, requested service ID, and requested contract ID.
   *
   * @param access_token A valid access token that is associated with the
   * service.
   * @param request Еру сontext of the `Authorize` request.
   * @param callback The`AuthorizeCallback` method that is called when
   * the `Authorize` request is completed.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken Authorize(std::string access_token,
                                      AuthorizeRequest request,
                                      AuthorizeCallback callback);

  /**
   * @brief Retrieves the account information associated with the user access
   * token.
   *
   * @note The user information will be filtered based on the scopes in
   * the token:
   * email - 'email', 'emailVerified', 'recoveryEmail';
   * openid - 'userId', 'state';
   * phone - 'phoneNumber', 'phoneNumberVerified';
   * profile - 'realm', 'facebookId', 'firstname', 'lastname', 'dob',
   * 'language', 'countryCode', 'marketingEnabled', 'createdTime',
   * 'updatedTime'.
   *
   * @param access_token A valid access token that is associated with the
   * user.
   * @param callback The`UserAccountInfoCallback` method that is called when
   * the user information request is completed.
   *
   * @return The `CancellationToken` instance that can be used to cancel
   * the request.
   */
  client::CancellationToken GetMyAccount(std::string access_token,
                                         UserAccountInfoCallback callback);

 private:
  std::unique_ptr<AuthenticationClientImpl> impl_;
};

}  // namespace authentication
}  // namespace olp
