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

#include <initializer_list>
#include <string>
#include <vector>

#include <boost/optional.hpp>
#include <olp/dataservice/read/DataServiceReadApi.h>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Represents one stream layer consumer config entry.
 *
 * An entry can be one of: string, int32, bool.
 */
class DATASERVICE_READ_API ConsumerOption final {
 public:
  ConsumerOption(std::string key, std::string value)
      : key_{std::move(key)}, value_{std::move(value)} {}

  ConsumerOption(std::string key, const char* value)
      : key_{std::move(key)}, value_{value} {}

  ConsumerOption(std::string key, int32_t value)
      : key_{std::move(key)}, value_{std::to_string(value)} {}

  ConsumerOption(std::string key, bool value)
      : key_{std::move(key)}, value_{std::to_string(value)} {}

  const std::string& GetKey() const { return key_; };
  const std::string& GetValue() const { return value_; };

 private:
  std::string key_;
  std::string value_;
};

using ConsumerOptions = std::vector<ConsumerOption>;

/**
 * @brief Holds all Kafka consumer properties to be passed to the Stream API.
 */
class DATASERVICE_READ_API ConsumerProperties final {
 public:
  ConsumerProperties(ConsumerOptions properties)
      : properties_{std::move(properties)} {}

  ConsumerProperties(std::initializer_list<ConsumerOption> properties)
      : properties_{std::move(properties)} {}

  const ConsumerOptions& GetProperties() const {
    return properties_;
  };

 private:
  ConsumerOptions properties_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
