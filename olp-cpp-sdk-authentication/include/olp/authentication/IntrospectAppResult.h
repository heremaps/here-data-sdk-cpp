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

#pragma once

#include <ctime>
#include <string>
#include <vector>

#include <olp/authentication/AuthenticationApi.h>

namespace olp {
namespace authentication {
/**
 * @brief A response to a client or user introspect application operation.
 */
class AUTHENTICATION_API IntrospectAppResult {
 public:
  /**
   * @brief Gets the identifier for the client or application.
   *
   * @return The client identifier.
   */
  const std::string& GetClientId() const { return client_id_; }

  /**
   * @brief Sets the identifier for the client or application.
   *
   * @param client_id The client identifier.
   */
  void SetClientId(std::string client_id) { client_id_ = std::move(client_id); }

  /**
   * @brief Gets the human-readable name of the client.
   *
   * @return The client name.
   */
  const std::string& GetName() const { return name_; }

  /**
   * @brief Sets the name of the client.
   *
   * @param name The client name.
   */
  void SetName(std::string name) { name_ = std::move(name); }

  /**
   * @brief Gets the prose description of the client.
   *
   * @return The client description.
   */
  const std::string& GetDescription() const { return description_; }

  /**
   * @brief Sets the prose description of the client.
   *
   * @param description The client description.
   */
  void SetDescription(std::string description) {
    description_ = std::move(description);
  }

  /**
   * @brief Gets a list of redirect URIs.
   *
   * Should be fully qualified HTTPS URIs without any fragments (HTTP is
   * only supported for the localhost development). At least one redirect URI
   * should be registered if the response types are non-empty.
   *
   * @return The list of the redirect URIs.
   */
  const std::vector<std::string>& GetRedirectUris() const {
    return redirect_uris_;
  }

  /**
   * @brief Sets the redirect URIs.
   *
   * @param uris The list of the redirect URIs.
   */
  void SetReditectUris(std::vector<std::string> uris) {
    redirect_uris_ = std::move(uris);
  }

  /**
   * @brief Gets a list of strings that represent scopes.
   *
   * This field is required when the response types are non-empty.
   *
   * @return The list of strings that represent the scopes.
   */
  const std::vector<std::string>& GetAllowedScopes() const {
    return allowed_scopes_;
  }

  /**
   * @brief Sets the list of the scopes.
   *
   * @param scopes The list of strings that represent the scopes.
   */
  void SetAllowedScopes(std::vector<std::string> scopes) {
    allowed_scopes_ = std::move(scopes);
  }

  /**
   * @brief Gets the token endpoint authentication method.
   *
   * @return The token endpoint authentication method.
   */
  const std::string& GetTokenEndpointAuthMethod() const {
    return token_endpoint_auth_method_;
  }

  /**
   * @brief Sets the token endpoint authentication method.
   *
   * @param method The token endpoint authentication method.
   */
  void SetTokenEndpointAuthMethod(std::string method) {
    token_endpoint_auth_method_ = std::move(method);
  }

  /**
   * @brief Gets the token endpoint authentication method reason.
   *
   * @return The token endpoint authentication method reason.
   */
  const std::string& GetTokenEndpointAuthMethodReason() const {
    return token_endpoint_auth_method_reason_;
  }

  /**
   * @brief Sets token endpoint authentication method reason.
   *
   * @param reason The token endpoint authentication method reason.
   */
  void SetTokenEndpointAuthMethodReason(std::string reason) {
    token_endpoint_auth_method_reason_ = std::move(reason);
  }

  /**
   * @brief Gets the DOB requirement.
   *
   * @return If true, the user must provide their DOB; if false, the user
   * can use self-service and specify "yes I am over age X".
   */
  bool GetDobRequired() const { return dob_required_; }

  /**
   * @brief Sets the DOB requirement.
   *
   * @param is_required If true, the user must provide their DOB; if false, the user
   * can use self-service and specify "yes I am over age X".
   */
  void SetDobRequired(bool is_required) { dob_required_ = is_required; }

  /**
   * @brief Gets the default duration in seconds for the token issued to this
   * application.
   *
   * It has to be a non-zero value less than or equal to 72 hours (259200).
   *
   * @return The token duration in seconds.
   */
  int GetTokenDuration() const { return token_duration_; }

  /**
   * @brief Sets the default duration in seconds for the token.
   *
   * @param duration The token duration in seconds.
   */
  void SetTokenDuration(int duration) { token_duration_ = duration; }

  /**
   * @brief Gets a list of referrer URLs to register or
   * that were registered with the application.
   *
   * When creating or updating: if the parameter is not present, the current value(s)
   * are unchanged. If an empty list is specified, the current value(s) will be reset to
   * the empty list.
   *
   * A value can contain 1-255 characters.
   * Wildcards are NOT allowed. The only valid characters are alphanumerics, '-',
   * '_', '.', '/'. The protocol is NOT specified,
   * that is, `http://` or `https://` is not allowed.
   *
   * Examples: `here.com`, `localhost`, `127.0.0.1`,
   * `www.facebook.com/hello/world/`.
   *
   * @return The list of referrer URLs to register or
   * that were registered with the application.
   */
  const std::vector<std::string>& GetReferrers() const { return referrers_; }

  /**
   * @brief Sets the list of referrer URLs to register or
   * that were registered with the application.
   *
   * @param urls The llist of referrer URLs.
   */
  void SetReferrers(std::vector<std::string> urls) { referrers_ = std::move(urls); }

  /**
   * @brief Gets the status of the application.
   *
   * @return The status of the application.
   */
  const std::string& GetStatus() const { return status_; }

  /**
   * @brief Sets the status of the application.
   *
   * @param status The status of the application.
   */
  void SetStatus(std::string status) { status_ = std::move(status); }

  /**
   * @brief Checks if the application codes are enabled.
   *
   * @return True if the application codes are enabled; false otherwise.
   */
  bool GetAppCodeEnabled() const { return app_code_enabled_; }

  /**
   * @brief Sets the application codes if they are enabled.
   *
   * @param is_enabled True if the application codes are enabled; false otherwise.
   */
  void SetAppCodeEnabled(bool is_enabled) { app_code_enabled_ = is_enabled; }

  /**
   * @brief  Gets the timestamp (milliseconds since the Unix epoch) of when
   * the application was created.
   *
   * @return The epoch time when the application was created.
   */
  time_t GetCreatedTime() const { return created_time_; }

  /**
   * @brief Sets the timestamp (milliseconds since the Unix epoch) of when
   * the application was created.
   *
   * @param time The epoch time when the application was created.
   */
  void SetCreatedTime(time_t time) { created_time_ = time; }

  /**
   * @brief Gets the realm to which the application belongs.
   *
   * @return The application realm.
   */
  const std::string& GetRealm() const { return realm_; }

  /**
   * @brief Sets the realm to which the application belongs.
   *
   * @param realm The application realm.
   */
  void SetRealm(std::string realm) { realm_ = std::move(realm); }

  /**
   * @brief Gets the application type.
   *
   * @return The application type.
   */
  const std::string& GetType() const { return type_; }

  /**
   * @brief Sets the application type.
   *
   * @param type The application type.
   */
  void SetType(std::string type) { type_ = std::move(type); }

  /**
   * @brief Gets a list of response types.
   *
   * @return The list of response types.
   */
  const std::vector<std::string>& GetResponseTypes() const {
    return response_types_;
  }

  /**
   * @brief Sets the list of response types.
   *
   * @param types The list of response types.
   */
  void SetResponseTypes(std::vector<std::string> types) {
    response_types_ = std::move(types);
  }

  /**
   * @brief Gets a rate limit tier to configure the application.
   *
   * @return The application rate limit tier.
   */
  const std::string& GetTier() const { return tier_; }

  /**
   * @brief Sets the rate limit tier to configure the application.
   *
   * @param tier The application rate limit tier.
   */
  void SetTier(std::string tier) { tier_ = std::move(tier); }

  /**
   * @brief Gets the HRN of the application.
   *
   * @return The application HRN.
   */
  const std::string& GetHRN() const { return hrn_; }

  /**
   * @brief Sets the HRN of the application.
   *
   * @param hrn The application HRN.
   */
  void SetHrn(std::string hrn) { hrn_ = std::move(hrn); }

 private:
  std::string client_id_;
  std::string name_;
  std::string description_;
  std::vector<std::string> redirect_uris_;
  std::vector<std::string> allowed_scopes_;
  std::string token_endpoint_auth_method_;
  std::string token_endpoint_auth_method_reason_;
  bool dob_required_;
  int token_duration_;
  std::vector<std::string> referrers_;
  std::string status_;
  bool app_code_enabled_;
  time_t created_time_;
  std::string realm_;
  std::string type_;
  std::vector<std::string> response_types_;
  std::string tier_;
  std::string hrn_;
};
}  // namespace authentication
}  // namespace olp
