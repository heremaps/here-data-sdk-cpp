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

#include <string>

#include <boost/optional/optional.hpp>
#include "AuthenticationApi.h"

namespace olp {
namespace authentication {
/**
 * @brief The access key ID and access key secret that you got from the HERE
 * Account as a part of the onboarding or support process on the developer
 * portal.
 *
 * Your credentials can be read in the following 2 ways:
 * * From a stream using the \ref ReadFromStream
 * method.
 * * From the **credentials.properties** file using
 * the \ref ReadFromFile method.
 *
 * For instructions on how to get the access keys, see
 * the [Get
 * Credentials](https://developer.here.com/olp/documentation/access-control/user-guide/topics/get-credentials.html)
 * section in the Terms and Permissions User Guide.
 */
class AUTHENTICATION_API AuthenticationCredentials {
 public:
  /**
   * @brief Reads your access credentials from an input stream that is
   * interpreted as a sequence of characters and retrieves a value with your
   * credentials.
   *
   * The stream must contain the following sequences of characters:
   * * here.access.key.id &ndash; your access key ID
   * * here.access.key.secret &ndash; your access key secret
   *
   * @param[in] stream The stream from which the credentials are read.
   *
   * @return An optional value with your credentials if the credentials were
   * read successfully, or no value in case of failure.
   */
  static boost::optional<AuthenticationCredentials> ReadFromStream(
      std::istream& stream);

  /**
   * @brief Parses the **credentials.properties** file downloaded from the [Open
   * Location Platform (OLP)](https://platform.here.com/)
   * website and retrieves a value with your credentials.
   *
   * The file must contain the following lines:
   * * here.access.key.id &ndash; your access key ID
   * * here.access.key.secret &ndash; your access key secret
   *
   * For instructions on how to get the **credentials.properties** file, see
   * the [Get
   * Credentials](https://developer.here.com/olp/documentation/access-control/user-guide/topics/get-credentials.html)
   * section in the Terms and Permissions User Guide.
   *
   * @param[in] filename The path to the file that contains the credentials.
   * An empty path is replaced with the following default path:
   * `$HOME/.here/credentials.properties`
   *
   * @return The optional value with your credentials if the credentials were
   * read successfully, or no value in case of failure.
   */
  static boost::optional<AuthenticationCredentials> ReadFromFile(
      std::string filename = {});
  AuthenticationCredentials() = delete;

  /**
   * @brief Creates the `AuthenticationCredentials` instance with your access
   * key ID and access key secret.
   *
   * @param key Your access key ID.
   * @param secret Your access key secret.
   */
  AuthenticationCredentials(std::string key, std::string secret);

  /**
   * @brief Creates the `AuthenticationCredentials` instance that is a copy
   * of the `other` parameter that contains the access credentials.
   *
   * @param other The `AuthenticationCredentials` instance from which the access
   * key ID and access key secret are copied.
   */
  AuthenticationCredentials(const AuthenticationCredentials& other);

  /**
   * @brief Gets the access key ID from the `AuthenticationCredentials`
   * instance.
   *
   * @return The const reference to the access key ID member.
   */
  const std::string& GetKey() const;

  /**
   * @brief Gets the access key secret from the `AuthenticationCredentials`
   * instance.
   *
   * @return The const reference to the access key secret member.
   */
  const std::string& GetSecret() const;

 private:
  std::string key_;
  std::string secret_;
};

}  // namespace authentication
}  // namespace olp
