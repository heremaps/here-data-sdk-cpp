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

#include "AuthenticationApi.h"

namespace olp {
namespace authentication {
namespace model {
/**
 * @brief An account information.
 */
class AUTHENTICATION_API UserAccountInfoResponse {
 public:
  /**
   * @brief Gets the HERE Account ID of the user.
   *
   * @return The HERE Account ID of the user.
   */
  const std::string& GetUserId() const { return user_id_; }

  /**
   * @brief Sets the HERE Account ID of the user.
   *
   * @param user_id The HERE Account ID of the user.
   */
  void SetUserId(std::string user_id) { user_id_ = std::move(user_id); }

  /**
   * @brief Gets the realm in which the user account exists.
   *
   * @return The user realm.
   */
  const std::string& GetRealm() const { return realm_; }

  /**
   * @brief Sets the realm in which the user account exists.
   *
   * @param realm The user realm.
   */
  void SetRealm(std::string realm) { realm_ = std::move(realm); }

  /**
   * @brief Gets the Facebook ID of the user.
   *
   * @note Not empty only if the account was created as a result of signing up
   * with a Facebook token.
   *
   * @return The Facebook ID of the user.
   */
  const std::string& GetFacebookId() const { return facebook_id_; }

  /**
   * @brief Sets the Facebook ID of the user.
   *
   * @param facebook_id The Facebook ID of the user.
   */
  void SetFacebookId(std::string facebook_id) {
    facebook_id_ = std::move(facebook_id);
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
   * @param lastname The last name of the user.
   */
  void SetLastname(std::string lastname) { lastname_ = std::move(lastname); }

  /**
   * @brief Gets the primary email address.
   *
   * @return The primary email address.
   */
  const std::string& GetEmail() const { return email_; }

  /**
   * @brief Sets the primary email address.
   *
   * @param email The primary email address.
   */
  void SetEmail(std::string email) { email_ = std::move(email); }

  /**
   * @brief Gets the recovery email address.
   *
   * @return The recovery email address.
   */
  const std::string& GetRecoveryEmail() const { return recovery_email_; }

  /**
   * @brief Sets the recovery email address.
   *
   * @param recovery_email The recovery email address.
   */
  void SetRecoveryEmail(std::string recovery_email) {
    recovery_email_ = std::move(recovery_email);
  }

  /**
   * @brief Gets the day of birth in the day/month/year format.
   *
   * @return The day of birth.
   */
  const std::string& GetDob() const { return dob_; }

  /**
   * @brief Sets the day of birth.
   *
   * @param dob The day of birth.
   */
  void SetDob(std::string dob) { dob_ = std::move(dob); }

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
   * @param country_code The country code.
   */
  void SetCountryCode(std::string country_code) {
    country_code_ = std::move(country_code);
  }

  /**
   * @brief Gets the code of the user's language in the ISO 639-1 2 format.
   *
   * @return The language code.
   */
  const std::string& GetLanguage() const { return language_; }

  /**
   * @brief Sets the code of the user's language in the ISO 639-1 2 format.
   *
   * @param language The language code.
   */
  void SetLanguage(std::string language) { language_ = std::move(language); }

  /**
   * @brief Gets the verification of the primary email address.
   *
   * @note The user is asked to verify their email on signup, but doing so is
   * optional.
   *
   * @return True if the primary email address was verified; false otherwise.
   */
  bool GetEmailVerified() const { return email_verified_; }

  /**
   * @brief Sets the verification of the primary email address.
   *
   * @param email_verified True if the primary email address was verified; false
   * otherwise.
   */
  void SetEmailVerified(bool email_verified) {
    email_verified_ = email_verified;
  }

  /**
   * @brief Gets the user's phone number.
   *
   * @return The phone number.
   */
  const std::string& GetPhoneNumber() const { return phone_number_; }

  /**
   * @brief Sets the user's phone number.
   *
   * @param phone_number The phone number.
   */
  void SetPhoneNumber(std::string phone_number) {
    phone_number_ = phone_number;
  }

  /**
   * @brief Gets the verification of the phone number.
   *
   * @return True if the phone number was verified; false otherwise.
   */
  bool GetPhoneNumberVerified() const { return phone_number_verified_; }

  /**
   * @brief Sets the verification of the phone number.
   *
   * @param phone_number_verified True if the phone number was verified; false
   * otherwise.
   */
  void SetPhoneNumberVerified(bool phone_number_verified) {
    phone_number_verified_ = phone_number_verified;
  }

  /**
   * @brief Checks if the marketing is enabled.
   *
   * @return True if the marketing is enabled; false otherwise.
   */
  bool GetMarketingEnabled() const { return marketing_enabled_; }

  /**
   * @brief Sets the marketing if it is enabled.
   *
   * @param marketing_enabled True if the marketing is enabled; false otherwise.
   */
  void SetMarketingEnabled(bool marketing_enabled) {
    marketing_enabled_ = marketing_enabled;
  }

  /**
   * @brief Gets the timestamp (milliseconds since the Unix epoch) of when
   * the account was created.
   *
   * @return The epoch time when the account was created.
   */
  time_t GetCreatedTime() const { return created_time_; }

  /**
   * @brief Sets the timestamp (milliseconds since the Unix epoch) of when
   * the account was created.
   *
   * @param time The epoch time when the account was created.
   */
  void SetCreatedTime(time_t created_time) { created_time_ = created_time; }

  /**
   * @brief Gets the timestamp (milliseconds since the Unix epoch) of when the
   * account was last updated.
   *
   * @note The time is updated when the following
   * user properties are changed: `email`, `recoveryEmail`, `dob`,
   * `countryCode`, `firstname`, `lastname`, `language`, `phoneNumber`.
   *
   * @return The epoch time when the account was updated.
   */
  time_t GetUpdatedTime() const { return updated_time_; }

  /**
   * @brief Sets the timestamp (milliseconds since the Unix epoch) of when the
   * account was last updated.
   *
   * @return The epoch time when the account was updated.
   */
  void SetUpdatedTime(time_t updated_time) { updated_time_ = updated_time; }

  /**
   * @brief Gets the current state of the account.
   *
   * @note The list of the possible states of the account:
   * 'deleted' - The account was permanently deleted.
   * 'disabled' - The account was disabled by an administrator.
   * 'locked' - The account was automatically locked due to suspicious activity
   * and will be unlocked after some time.
   * 'enabled' - The account is enabled, and the user can log in.
   *
   * @return The current state of the account.
   */
  const std::string& GetState() const { return state_; }

  /**
   * @brief Sets the current state of the account.
   *
   * @param state The current state of the account.
   */
  void SetState(std::string state) { state_ = std::move(state); }

  /**
   * @brief Gets the HRN of the user.
   *
   * @return The user HRN.
   */
  const std::string& GetHrn() const { return hrn_; }

  /**
   * @brief Sets the HRN of the user.
   *
   * @param hrn The user HRN.
   */
  void SetHrn(std::string hrn) { hrn_ = std::move(hrn); }

  /**
   * @brief Gets the account type.
   *
   * @return The account type.
   */
  const std::string& GetAccountType() const { return account_type_; }

  /**
   * @brief Sets the account type.
   *
   * @param account_type The account type.
   */
  void SetAccountType(std::string account_type) {
    account_type_ = std::move(account_type);
  }

 private:
  std::string user_id_;
  std::string realm_;
  std::string facebook_id_;
  std::string firstname_;
  std::string lastname_;
  std::string email_;
  std::string recovery_email_;
  std::string dob_;
  std::string country_code_;
  std::string language_;
  bool email_verified_;
  std::string phone_number_;
  bool phone_number_verified_;
  bool marketing_enabled_;
  time_t created_time_;
  time_t updated_time_;
  std::string state_;
  std::string hrn_;
  std::string account_type_;
};
}  // namespace model
}  // namespace authentication
}  // namespace olp
