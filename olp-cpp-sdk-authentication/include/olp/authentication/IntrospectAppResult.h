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
 * @brief A response to a client or user introspect app operation.
 */
class AUTHENTICATION_API IntrospectAppResult {
 public:
  /**
   * @brief Identifier for the client/application.
   *
   * @return The string containing client identifier.
   */
  const std::string& GetClientId() const { return client_id_; }

  /**
   * @brief Sets identifier for the client/application.
   *
   * @param client_id The string containing client identifier.
   */
  void SetClientId(std::string client_id) { client_id_ = std::move(client_id); }

  /**
   * @brief Human readable name of the client.
   *
   * @return The string containing client name.
   */
  const std::string& GetName() const { return name_; }

  /**
   * @brief Sets the name of the client.
   *
   * @param name The string containing client name.
   */
  void SetName(std::string name) { name_ = std::move(name); }

  /**
   * @brief Prose description of the client.
   *
   * @return The string containing client description.
   */
  const std::string& GetDescription() const { return description_; }

  /**
   * @brief Sets prose description of the client.
   *
   * @param description The string containing client description.
   */
  void SetDescription(std::string description) {
    description_ = std::move(description);
  }

  /**
   * @brief List of redirect uris.
   *
   * Should be fully qualified HTTPS uris without any fragments (HTTP is
   * only supported for localhost development). At least one redirect uri
   * should be registered if the response types is non-empty.
   *
   * @return The list of redirect uris.
   */
  const std::vector<std::string>& GetRedirectUris() const {
    return redirect_uris_;
  }

  /**
   * @brief Sets the redirect uris.
   *
   * @param uris The list of redirect uris.
   */
  void SetReditectUris(std::vector<std::string> uris) {
    redirect_uris_ = std::move(uris);
  }

  /**
   * @brief List of strings representing the scopes.
   *
   * This field is required when the response types is non-empty.
   *
   * @return The list of strings representing the scopes.
   */
  const std::vector<std::string>& GetAllowedScopes() const {
    return allowed_scopes_;
  }

  /**
   * @brief Sets list of the scopes.
   *
   * @param scopes The list of strings representing the scopes.
   */
  void SetAllowedScopes(std::vector<std::string> scopes) {
    allowed_scopes_= std::move(scopes);
  }

  /**
   * @brief Token endpoint authentication method.
   *
   * The default value will be "client_secret_jwt".
   *
   * @return The token endpoint authentication method.
   */
  const std::string& GetTokenEndpointAuthMethod() const {
    return token_endpoint_auth_method_;
  }

  /**
   * @brief Sets token endpoint authentication method.
   *
   * @param method The token endpoint authentication method.
   */
  void SetTokenEndpointAuthMethod(std::string method) {
    token_endpoint_auth_method_ = std::move(method);
  }

  /**
   * @brief Token endpoint authentication method reason.
   *
   * @return The token endpoint authentication reason.
   */
  const std::string& GetTokenEndpointAuthMethodReason() const {
    return token_endpoint_auth_method_reason_;
  }

  /**
   * @brief Sets token endpoint authentication method reason.
   *
   * @param reason The token endpoint authentication reason.
   */
  void SetTokenEndpointAuthMethodReason(std::string reason) {
    token_endpoint_auth_method_reason_= std::move(reason);
  }

  /**
   * @brief Identifies DOB requirement.
   *
   * @return True - the user must supply their DOB, false - the user
   * can use self-service and indicate "yes I am over age X".
   */
  bool GetDobRequired() const { return dob_required_; }

  /**
   * @brief Sets DOB requirement.
   *
   * @param is_required True - the user must supply their DOB, false - the user
   * can use self-service and indicate "yes I am over age X".
   */
  void SetDobRequired(bool is_required) { dob_required_ = is_required; }

  /**
   * @brief default duration in seconds for the token issued to this
   * application.
   *
   * It has be a non-zero value less than or equal to 72 hours (259200).
   *
   * @return The token duration in seconds.
   */
  int GetTokenDuration() const { return token_duration_; }

  /**
   * @brief Sets default duration in seconds for the token.
   *
   * @param duration The token duration in seconds.
   */
  void SetTokenDuration(int duration) { token_duration_ = duration; }

  /**
   * @brief List referrer urls to register/registered with Application.
   *
   * On create/update: if the parameter is not present, the current value(s)
   * are unchanged. If empty list specified, current value(s) will be reset to
   * empty list.
   * Value has a min length of 1 char and max of 255 chars.
   * Wildcards are NOT allowed. The only valid characters: alphanumerics, '-',
   * '_', '.', '/' The protocol is NOT specified, i.e. no http:// or https://
   * Some examples: here.com, localhost, 127.0.0.1,
   * www.facebook.com/hello/world/.
   *
   * @return The list referrer urls to register/registered with Application.
   */
  const std::vector<std::string>& GetReferrers() const { return referrers_; }

  /**
   * @brief Sets list referrer urls to register/registered with Application.
   *
   * @param urls The list referrer urls.
   */
  void SetReferrers(std::vector<std::string> urls) { referrers_ = std::move(urls); }

  /**
   * @brief Status of the application.
   *
   * @return The status of the application.
   */
  const std::string& GetStatus() const { return status_; }

  /**
   * @brief Sets status of the application.
   *
   * @param status The status of the application.
   */
  void SetStatus(std::string status) { status_ = std::move(status); }

  /**
   * @brief Identifies if application codes are enabled.
   *
   * @return True if application codes are enabled and false otherwise.
   */
  bool GetAppCodeEnabled() const { return app_code_enabled_; }

  /**
   * @brief Sets is application codes are enabled.
   *
   * @param is_enabled True if application codes are enabled and false otherwise.
   */
  void SetAppCodeEnabled(bool is_enabled) { app_code_enabled_ = is_enabled; }

  /**
   * @brief Timestamp (milliseconds since the Unix epoch) of when
   * the application was created.
   *
   * @return Epoch time when the application was created.
   */
  time_t GetCreatedTime() const { return created_time_; }

  /**
   * @brief Sets timestamp (milliseconds since the Unix epoch) of when
   * the application was created.
   *
   * @param time The epoch time when the application was created.
   */
  void SetCreatedTime(time_t time) { created_time_ = time; }

  /**
   * @brief Realm the application belongs to.
   *
   * @return The string containing application realm.
   */
  const std::string& GetRealm() const { return realm_; }

  /**
   * @brief Sets realm the application belongs to.
   *
   * @param realm The application realm.
   */
  void SetRealm(std::string realm) { realm_ = std::move(realm); }

  /**
   * @brief Type of the application.
   *
   * @return The type of the application.
   */
  const std::string& GetType() const { return type_; }

  /**
   * @brief Sets type of the application.
   *
   * @param type The type of the application.
   */
  void SetType(std::string type) { type_ = std::move(type); }

  /**
   * @brief List of response types.
   *
   * @return The list of response types.
   */
  const std::vector<std::string>& GetResponseTypes() const {
    return response_types_;
  }

  /**
   * @brief Sets list of response types.
   *
   * @param types The list of response types.
   */
  void SetResponseTypes(std::vector<std::string> types) {
    response_types_ = std::move(types);
  }

  /**
   * @brief Rate limit tier to configure application for.
   *
   * @return The string containing application rate limit tier.
   */
  const std::string& GetTier() const { return tier_; }

  /**
   * @brief Sets rate limit tier to configure application for.
   *
   * @param tier The application rate limit tier.
   */
  void SetTier(std::string tier) { tier_ = std::move(tier); }

  /**
   * @brief HRN of the application.
   *
   * @return The string with application HRN.
   */
  const std::string& GetHRN() const { return hrn_; }

  /**
   * @brief Sets HRN of the application.
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
