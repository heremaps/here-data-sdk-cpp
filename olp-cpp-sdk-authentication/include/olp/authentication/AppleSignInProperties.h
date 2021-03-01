/*
 * Copyright (C) 2021 HERE Europe B.V.
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

#include <olp/authentication/AuthenticationApi.h>

namespace olp {
namespace authentication {

/**
 * @brief The Apple sign-in properties.
 */
class AUTHENTICATION_API AppleSignInProperties {
 public:
  /**
   * @brief Gets the bearer in the authorization request header.
   *
   * @note This is the Apple 'id_token' received after authorization.
   *
   * @return The access token.
   */
  const std::string& GetAccessToken() const { return access_token_; }

  /**
   * @brief Sets the bearer in the authorization request header.
   *
   * @note This is the Apple 'id_token' received after authorization.
   *
   * @param access_token The access token.
   */
  void SetAccessToken(std::string access_token) {
    access_token_ = std::move(access_token);
  }

  /**
   * @brief Gets the first name of the user.
   *
   * @return The first name of the user.
   */
  const std::string& GetFirstname() const { return firstname_; }

  /**
   * @brief Sets the first name of the user.
   *
   * Required for the first-time sign-in; optional for the subsequent sign-ins.
   *
   * @param firstname The first name of the user.
   */
  void SetFirstname(std::string firstname) {
    firstname_ = std::move(firstname);
  }

  /**
   * @brief Gets the last name of the user.
   *
   * @return The last name of the user.
   */
  const std::string& GetLastname() const { return lastname_; }

  /**
   * @brief Sets the last name of the user.
   *
   * Required for the first-time sign-in; optional for the subsequent sign-ins.
   *
   * @param lastname The last name of the user.
   */
  void SetLastname(std::string lastname) { lastname_ = std::move(lastname); }

  /**
   * @brief Gets the code of the user's country in the ISO 3166-1 alpha-3
   * format.
   *
   * @return The country code.
   */
  const std::string& GetCountryCode() const { return country_code_; }

  /**
   * @brief Sets the code of the user's country in the ISO 3166-1 alpha-3
   * format.
   *
   * Required for the first-time sign-in; optional for the subsequent sign-ins.
   *
   * @param country_code The country code.
   */
  void SetCountryCode(std::string country_code) {
    country_code_ = std::move(country_code);
  }

  /**
   * @brief Gets the code of the user's language in the ISO 639-1 alpha-2
   * format.
   *
   * @return The language code.
   */
  const std::string& GetLanguage() const { return language_; }

  /**
   * @brief Sets the code of the user's language in the ISO 639-1 alpha-2
   * format.
   *
   * Required for the first-time sign-in; optional for the subsequent sign-ins.
   *
   * @param language The language code.
   */
  void SetLanguage(std::string language) { language_ = std::move(language); }

  /**
   * @brief Gets the application ID.
   *
   * @return The application ID.
   */
  const std::string& GetClientId() const { return client_id_; }

  /**
   * @brief Sets the application ID.
   *
   * @param client_id The application ID.
   */
  void SetClientId(std::string client_id) { client_id_ = std::move(client_id); }

  /**
   * @brief Gets the platform realm to which the application ID (`client_id`)
   * belongs.
   *
   * @return The client realm.
   */
  const std::string& GetRealm() const { return realm_; }

  /**
   * @brief Sets the platform realm to which the application ID (`client_id`)
   * belongs.
   *
   * @param realm The client realm.
   */
  void SetRealm(std::string realm) { realm_ = std::move(realm); }

 private:
  std::string access_token_;
  std::string firstname_;
  std::string lastname_;
  std::string country_code_;
  std::string language_;
  std::string client_id_;
  std::string realm_;
};

}  // namespace authentication
}  // namespace olp
