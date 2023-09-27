/*
 * Copyright (C) 2023 HERE Europe B.V.
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

#include <olp/core/utils/Credentials.h>

namespace {
void CensorCredentialsPart(std::string& url, std::size_t arguments_start,
                           const std::string& credentials_part_name) {
  const auto credentials_part_argument_start =
      url.find(credentials_part_name, arguments_start);

  if (credentials_part_argument_start == std::string::npos) {
    return;
  }

  const auto credentials_part_value_start =
      credentials_part_argument_start + credentials_part_name.length();
  const auto credentials_part_argument_end =
      url.find('&', credentials_part_value_start);
  const auto credentials_part_value_end =
      credentials_part_argument_end != std::string::npos
          ? credentials_part_argument_end
          : url.size();

  const auto credentials_part_value_begin_it =
      std::next(url.begin(), credentials_part_value_start);
  const auto credentials_part_value_end_it =
      std::next(url.begin(), credentials_part_value_end);
  std::fill(credentials_part_value_begin_it, credentials_part_value_end_it,
            '*');
}
}  // anonymous namespace

namespace olp {
namespace utils {
std::string CensorCredentialsInUrl(std::string url) {
  const auto arguments_start_pos = url.find('?');

  if (arguments_start_pos == std::string::npos) {
    return url;
  }

  static const std::string app_id_name = "app_id=";
  CensorCredentialsPart(url, arguments_start_pos, app_id_name);

  static const std::string app_code_name = "app_code=";
  CensorCredentialsPart(url, arguments_start_pos, app_code_name);

  static const std::string api_key_name = "apiKey=";
  CensorCredentialsPart(url, arguments_start_pos, api_key_name);

  return url;
}

}  // namespace utils
}  // namespace olp
