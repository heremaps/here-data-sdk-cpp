/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include "olp/authentication/AuthenticationCredentials.h"

#include <cstdlib>
#include <fstream>
#include <regex>
#include <vector>

namespace {
const std::string kRegexString = "\\s*=\\s*";
const std::string kHereAccessKeyId = "here.access.key.id";
const std::string kHereAccessKeySecret = "here.access.key.secret";
const std::string kHereTokenEndpointUrl = "here.token.endpoint.url";

/// Forms a default path that is valid for the current OS.
std::string GetDefaultPath() {
  const char* env;
#ifdef _WIN32
  if ((env = std::getenv("USERPROFILE")) != nullptr) {
    return std::string(env).append("\\.here\\credentials.properties");
  }

  if ((env = std::getenv("HOMEDRIVE")) != nullptr) {
    std::string path{env};

    if ((env = std::getenv("HOMEPATH")) != nullptr) {
      path.append(env).append("\\.here\\credentials.properties");
      return path;
    }
  }
#else
  if ((env = std::getenv("HOME"))) {
    return std::string(env) + "/.here/credentials.properties";
  }
#endif  // _WIN32
  return {};
}
}  // namespace

namespace olp {
namespace authentication {

boost::optional<AuthenticationCredentials>
AuthenticationCredentials::ReadFromStream(std::istream& stream) {
  static const std::regex kRegex{kRegexString};

  std::string access_key_id;
  std::string access_key_secret;
  std::string token_endpoint_url;
  std::string line;

  while (std::getline(stream, line)) {
    // Handle both CRLF and LF line endings
    line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

    const std::vector<std::string> token{
        std::sregex_token_iterator(line.begin(), line.end(), kRegex, -1),
        std::sregex_token_iterator()};

    if (token.size() != 2) {
      continue;
    }

    if (token[0] == kHereAccessKeyId) {
      access_key_id = token[1];
    } else if (token[0] == kHereAccessKeySecret) {
      access_key_secret = token[1];
    } else if (token[0] == kHereTokenEndpointUrl) {
      token_endpoint_url = token[1];
    }
  }

  return (!access_key_id.empty() && !access_key_secret.empty())
             ? boost::make_optional<AuthenticationCredentials>(
                   {std::move(access_key_id), std::move(access_key_secret),
                    std::move(token_endpoint_url)})
             : boost::none;
}

boost::optional<AuthenticationCredentials>
AuthenticationCredentials::ReadFromFile(std::string filename) {
  if (filename.empty()) {
    filename = GetDefaultPath();
  }

  std::ifstream stream(filename, std::ios::in);
  return stream ? ReadFromStream(stream) : boost::none;
}

AuthenticationCredentials::AuthenticationCredentials(std::string key,
                                                     std::string secret)
    : key_(std::move(key)), secret_(std::move(secret)) {}

AuthenticationCredentials::AuthenticationCredentials(std::string key,
                                                     std::string secret,
                                                     std::string endpoint_url)
    : key_(std::move(key)),
      secret_(std::move(secret)),
      endpoint_url_(std::move(endpoint_url)) {}

const std::string& AuthenticationCredentials::GetKey() const { return key_; }

const std::string& AuthenticationCredentials::GetSecret() const {
  return secret_;
}

const std::string& AuthenticationCredentials::GetEndpointUrl() const {
  return endpoint_url_;
}

}  // namespace authentication
}  // namespace olp
