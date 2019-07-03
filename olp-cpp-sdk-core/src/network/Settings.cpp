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

#include <olp/core/network/Settings.h>

#include <cstdlib>
#include <map>
#include <string>
#if defined(WIN32)
#include <stdlib.h>
#endif
#ifdef NETWORK_ANDROID
#include <sys/system_properties.h>
#endif

namespace olp {
namespace network {
namespace {
std::map<std::string, std::string> gSettings;
}

const std::string Settings::SetValue(std::string key, std::string value) {
  std::string& oldValue = gSettings[std::move(key)];
  std::swap(oldValue, value);
  return value;
}

const std::string Settings::GetEnvString(const char* envStr,
                                         const std::string& default_value) {
  if (!gSettings.empty()) {
    auto it = gSettings.find(envStr);
    if (it != gSettings.end()) return it->second;
  }
#if defined(NETWORK_ANDROID)
  // We should keep the Android case for reading os-specific parameters
  // described in
  // http://stackoverflow.com/questions/7183627/retrieve-android-os-build-system-properties-via-purely-native-android-app
  char value[PROP_VALUE_MAX] = "";
  size_t len;
  if ((len = __system_property_get(envStr, value)) == 0) return defaultStr;
  std::string varStr(value, len);
  return varStr;

#elif defined(WIN32)
  char* var;
  size_t len;
  if (_dupenv_s(&var, &len, envStr) || (len == 0)) return default_value;
  std::string varStr(var, len - 1);
  free(var);
  return varStr;
#else
  const char* var = std::getenv(envStr);
  if (!var) return default_value;
  return std::string(var);
#endif
}

int Settings::GetEnvInt(const char* envStr, int default_value) {
  std::string value = GetEnvString(envStr);
  if (value.empty())
    return default_value;
  else
    return std::atoi(value.c_str());
}

}  // namespace network
}  // namespace olp
