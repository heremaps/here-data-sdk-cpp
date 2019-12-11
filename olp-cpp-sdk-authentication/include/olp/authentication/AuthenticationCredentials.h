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
 * @brief The AuthenticationCredentials represent the access keys issued for the
 * client by the HERE Account as part of the onboarding or support process
 * on the developer portal.
 */
class AUTHENTICATION_API AuthenticationCredentials {
 public:
  /*
   * @brief Reads your access credentials from an input stream that is
   * interpreted as a sequence of characters.
   *
   * The stream must contain the following sequences of characters:
   * * here.access.key.id = <your access key ID>
   * * here.access.key.secret = <your access key secret>
   *
   * @param[in] stream The stream from which the credentials are read.
   * @return An optional value with your credentials if the credentials were
   * read successfully, or no value in case of failure.
   */
  static boost::optional<AuthenticationCredentials> ReadFromStream(
      std::istream& stream);

  /*
   * @brief Parses the **credentials.properties** file downloaded from the OLP
   * [website](https://developer.here.com/olp/documentation/access-control/user-guide/topics/get-credentials.html)
   * and retrieves a value with your credentials.
   *
   * The file must contain the following lines:
   * * here.access.key.id = <your access key ID>
   * * here.access.key.secret = <your access key secret>
   *
   * @param[in] filename The path to the file that contains the credentials.
   * An empty path is replaced with the following default path:
   * $HOME/.here/credentials.properties
   * @return An optional value with your credentials if the credentials were
   * read successfully, or no value in case of failure.
   */
  static boost::optional<AuthenticationCredentials> ReadFromFile(
      std::string filename = {});
  AuthenticationCredentials() = delete;

  /**
   * @brief Constructor
   * @param key The client access key identifier.
   * @param secret The client access key secret.
   */
  AuthenticationCredentials(std::string key, std::string secret);

  /**
   * @brief Copy Constructor
   * @param other object to copy from
   */
  AuthenticationCredentials(const AuthenticationCredentials& other);

  /**
   * @brief access the key of this AuthenticationCredentials object
   * @return a reference to the key member
   */
  const std::string& GetKey() const;

  /**
   * @brief access the secret of this AuthenticationCredentials object
   * @return a reference to the secret member
   */
  const std::string& GetSecret() const;

 private:
  std::string key_;
  std::string secret_;
};

}  // namespace authentication
}  // namespace olp
